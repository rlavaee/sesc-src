#require 'graphviz'

module CTMC

  class Bus
		attr_accessor :ranges
    def initialize
      @ranges = Array.new
    end

    def fill_first_empty_slot(point,length)
      #puts "adding interval (#{point}..#{point+length})"
      #puts @ranges.inspect
      from = 0
      @ranges.each_with_index  do |range,i|
        to = range.first
        if(from > point)
           point = from
        end

        if(to <= point)
          from = range.last
          next
        end

        if(point+length > to)
          from = range.last
          next
        end
       
        @ranges.insert(i,(point..point+length))
				return point+length

      end
      @ranges << ([point,from].max..[point,from].max+length)
			return [point,from].max+length

    end
  end

  class << self
    attr_accessor :phase
    attr_accessor :rdJobCount
    attr_accessor :wrJobCount
    attr_accessor :rdServTime
    attr_accessor :wrServTime
    attr_accessor :rdThinkTime
    attr_accessor :wrThinkTime
    attr_accessor :queueSize
    attr_accessor :nbanks
    attr_accessor :burstLength
		attr_accessor :rdCntBank
		attr_accessor :wrCntBank
  end



  class Edge
    attr_accessor :from
    attr_accessor :to
    attr_accessor :rate

    def initialize(from,to,rate)
      @from = from
      @to = to
      @rate = rate
    end

    def is_self_edge?
      @from.eql?(@to)
    end

    def to_s
      "[edge from #{@from} to #{@to} with rate #{@rate}]"
    end
  end


  class State


    attr_accessor :in_edges
    attr_accessor :out_edges
    attr_reader :readsInQ
    attr_reader :writesInQ

    def initialize(readsInQ=0,writesInQ=0)
      @readsInQ = readsInQ
      @writesInQ = writesInQ
      @in_edges = []
      @out_edges = []
    end

    def State.add_node(rds,wrs)
      state = State.new(rds,wrs)
      get_stored_node(state)
    end

    def State.get_stored_node(state)
      if(!@@nodeIndices.include?(state))
        #puts "adding node #{state}"
        @@nodeIndices[state]= @@nodes.size
        @@nodes << state
        return state
      else
        return @@nodes[@@nodeIndices[state]]
      end
    end

    #Hash needs this
    def eql?(state)
      (@readsInQ == state.readsInQ) and (@writesInQ == state.writesInQ)
    end

    #Hash needs this too
    def hash
      to_s.hash
    end

    def add_edge(to,rate)
      storedTo = State.get_stored_node(to) 
      edge = Edge.new(self,storedTo,rate)
      #raise "self edge found: #{edge}" if(edge.is_self_edge?)
      @out_edges << edge
      storedTo.in_edges << edge
    end


    def normalize_edges
      fout = @out_edges.inject(0) { |result,oe| result+oe.rate}
      @out_edges.each {|edge| edge.rate/=fout}
    end

    def State.normalize_all_edges
      @@nodes.each {|state| state.normalize_edges}
    end

    def add_service_edges
      #puts "adding service edges for #{self}"
      read_service_rates = []
      write_service_rates = []
      nsamples = 30
      nsamples.times.each do |sample|
          #puts "new sample"
          bank_nreads = Array.new(CTMC.nbanks,0)
          bank_nwrites = Array.new(CTMC.nbanks,0)

          @readsInQ.times.map {Random.rand(CTMC.rdCntBank[CTMC.nbanks-1])}.map do |rand|
						CTMC.rdCntBank.each_with_index do |value,bank|
            	bank_nreads[bank] +=1 if(rand<value)
						end
          end
          @writesInQ.times.map {Random.rand(CTMC.wrCntBank[CTMC.nbanks-1])}.map do |rand|
						CTMC.wrCntBank.each_with_index do |value,bank|
            	bank_nwrites[bank] +=1 if(rand<value)
						end
          end
          #puts bank_nwrites
          #puts bank_nreads
          bus = Bus.new
					last_read = 0
					last_write = 0
					(0..CTMC.nbanks-1).each do |bank|
						point = 0
						nreads = bank_nreads[bank]
            nwrites = bank_nwrites[bank]
						while(nreads+nwrites!=0) do
							rand = Random.rand(nreads+nwrites)
							if(rand<nreads)
								point+=CTMC.rdServTime
              	point=bus.fill_first_empty_slot(point,CTMC.burstLength)
								last_read = [last_read,point].max
								nreads-=1
							else
								point+=CTMC.wrServTime
                point = bus.fill_first_empty_slot(point,CTMC.burstLength)
								last_write = [last_write,point].max
								nwrites-=1
              end
            end

          end
          read_service_rates << @readsInQ.to_f / last_read if(last_read!=0)
          write_service_rates << @writesInQ.to_f / last_write if(last_write!=0)
					end

      avg_read_service_rate = read_service_rates.inject{ |sum, el| sum + el }.to_f / nsamples
      avg_write_service_rate = write_service_rates.inject{ |sum, el| sum + el }.to_f / nsamples
      add_edge(State.new(@readsInQ-1,@writesInQ),avg_read_service_rate) if(@readsInQ > 0)
      add_edge(State.new(@readsInQ,@writesInQ-1),avg_write_service_rate) if(@writesInQ > 0)

    end

    def add_edges
          add_service_edges

          rdArrivalRate = (CTMC.rdJobCount-@readsInQ).to_f/CTMC.rdThinkTime
          wrArrivalRate = (CTMC.wrJobCount-@writesInQ).to_f/CTMC.wrThinkTime
          if(@readsInQ+@writesInQ+1 < CTMC.queueSize)
            add_edge(State.new(@readsInQ+1,@writesInQ),rdArrivalRate) if (@readsInQ < CTMC.rdJobCount)
            add_edge(State.new(@readsInQ,@writesInQ+1),wrArrivalRate) if (@writesInQ < CTMC.wrJobCount)
          end

    end

    def dump_matlab_code
      #sum the flow out
      fout = @out_edges.inject(0) { |result,oe| result+oe.rate}
			@@mi << @@nodeIndices[self]+1
			@@mj << @@nodeIndices[self]+1
			@@mv << -fout

      #raise "For #{self}, flow out is not equal to one, it's #{fout}" if(fout!=1)
      @in_edges.each do |ie|
        @@mi << @@nodeIndices[ie.to]+1
        @@mj << @@nodeIndices[ie.from]+1
        @@mv << ie.rate
      end

    end

    def State.initialize_all
      @@nodes = Array.new
      @@nodeIndices = Hash.new
    end

    def State.add_all_edges
      @@nodes.each { |state| state.add_edges }
    end

    def State.dump_all_edges
      @@nodes.each { |state| state.dump_edges }
    end

    def State.dump_all_matlab_code
      @@mi = Array.new
      @@mj = Array.new
      @@mv = Array.new
      @@nodes[0..@@nodes.size-2].each {|state| state.dump_matlab_code}
      @@nodes.each_index {|i| @@mi << @@nodes.size; @@mj << i+1 ; @@mv << 1 }
      File.open(File.join(DIR,"ctmc#{CTMC.phase}.m"),"w") do |mf|
        mf.puts "I = #{@@mi};"
        mf.puts "J = #{@@mj};"
        mf.puts "V = #{@@mv};"
        mf.puts "P = sparse(I,J,V,#{@@nodes.size},#{@@nodes.size});"
        #mf.puts "P = P - speye(#{@@nodes.size});"
        mf.puts "Y = sparse([zeros(#{@@nodes.size-1},1); 1]);"
        mf.puts "X = P\\Y;"
        mf.puts "fid = fopen('answer#{CTMC.phase}.txt','w');"
        mf.puts "s = repmat('%f,',1,length(X));"
        mf.puts "s(end)=[]; %Remove trailing comma"
        mf.puts "disp(fprintf(fid,['Ans = [' s ']'], full(X)));"
        mf.puts "exit"
      end
    end

    def State.dump_perf_params
      File.open(File.join(DIR,"perf.csv"),"a") do |pf|
        pf.write "#{CTMC.phase}\t"
        rdThru=0
        wrThru=0
        @@nodes.each_with_index do |state,i|
            state.out_edges.each do |edge|
              rdThru += Ans[i]*edge.rate if(edge.to.readsInQ > edge.from.readsInQ)
              wrThru += Ans[i]*edge.rate if(edge.to.writesInQ > edge.from.writesInQ)
            end
        end
        pf.write "#{rdThru}\t"
        pf.write "#{wrThru}\t"

        rdUtil=0
        wrUtil=0
        @@nodes.each_with_index do |state,i|
          rdUtil+=Ans[i] if(state.readsInQ!=0)
          wrUtil+=Ans[i] if(state.writesInQ!=0)
        end
        pf.write "#{rdUtil}\t"
        pf.write "#{wrUtil}\t"
        rdQOcc=0
        wrQOcc=0

        @@nodes.each_with_index do |state,i|
          rdQOcc+=state.readsInQ*Ans[i]
          wrQOcc+=state.writesInQ*Ans[i]
        end
        pf.write "#{rdQOcc}\t"
        pf.write "#{wrQOcc}\t"
        rdArrival = 0
        wrArrival = 0
        @@nodes.each_with_index do |state,i|
          if(state.readsInQ+state.writesInQ < CTMC.queueSize)
            rdArrival+=Ans[i]*(CTMC.rdJobCount-state.readsInQ)/ CTMC.rdThinkTime
            wrArrival+=Ans[i]*(CTMC.wrJobCount-state.writesInQ)/ CTMC.wrThinkTime
          end
        end


        pf.write "#{rdQOcc/rdArrival}\t"
        pf.write "#{wrQOcc/wrArrival}\n"

      end
    end


    def dump_edges  
      puts "#{self}:"
      @out_edges.each {|edge| puts edge}
    end

    def to_s
      "(#{@readsInQ},#{@writesInQ})"
    end

  end

  def generate_chain
    #the initial states
    State.initialize_all
    init = State.add_node(0,0)


    State.add_all_edges
    #State.normalize_all_edges
  end

  def dump_matlab_code
    State.dump_all_matlab_code
  end

  def dump_perf_params
    f = File.open(File.join(DIR,"answer#{CTMC.phase}.txt"),"r") do |f|
      text = f.read
      eval(text.lines.first)
    end
    State.dump_perf_params
  end

  def dump_edges
    State.dump_all_edges
  end

end


include CTMC
DIR = ARGV[0]
f=File.open(File.join(DIR,"radix.csv.1"),"r") do |input_file|
  input_file.each_line do |line|
    inputs = line.split(' ')
    CTMC.phase = inputs[0].to_i
    CTMC.burstLength=4
    CTMC.rdJobCount=[inputs[1].to_f.round,1].max
    CTMC.wrJobCount=[inputs[2].to_f.round,1].max

    CTMC.rdServTime=inputs[3].to_f.round-CTMC.burstLength
    CTMC.wrServTime=inputs[4].to_f.round-CTMC.burstLength

    CTMC.rdThinkTime=inputs[5].to_f
    CTMC.wrThinkTime=inputs[6].to_f

    CTMC.nbanks=8

		CTMC.rdCntBank = []
		CTMC.wrCntBank = []
		(0..CTMC.nbanks-1).each do |bank|
			CTMC.rdCntBank << inputs[7+bank].to_i
		end
		
		(0..CTMC.nbanks-1).each do |bank|
			CTMC.wrCntBank << inputs[7+CTMC.nbanks+bank].to_i
		end

		CTMC.rdCntBank.each_with_index.inject(0) {|sum,(value,index)| sum+=value; CTMC.rdCntBank[index]=sum}
		CTMC.wrCntBank.each_with_index.inject(0) {|sum,(value,index)| sum+=value; CTMC.wrCntBank[index]=sum}
		CTMC.rdCntBank[CTMC.nbanks-1]+=1 if(CTMC.rdCntBank[CTMC.nbanks-1]==0)
		CTMC.wrCntBank[CTMC.nbanks-1]+=1 if(CTMC.wrCntBank[CTMC.nbanks-1]==0)
			

    #CTMC.queueSize=inputs[7].to_i
    CTMC.queueSize = 64

    CTMC.generate_chain
    #CTMC.dump_edges
    CTMC.dump_matlab_code
    system `matlab -nodesktop -nosplash -r "run('#{File.join(DIR,"ctmc#{CTMC.phase}.m")}')" > /dev/null`
    CTMC.dump_perf_params
  end
end

