#require 'graphviz'

module CTMC

  class Bus
    def initialize
      @ranges = Array.new
    end

    def fill_first_empty_slot(point,length)
      #puts "adding interval (#{point}..#{point+length})"
      #puts @ranges.inspect
      found = false
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
       
        found = true
        @ranges.insert(i,(point..point+length))
        break

      end
      @ranges << ([point,from].max..[point,from].max+length) if(!found)
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
    attr_accessor :busContention
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
      x= @readsInQ+@writesInQ
      service_rates = []
      nsamples = 30
      nsamples.times.inject do |sample|
          #puts "new sample"
          bank_nreads = {}
          bank_nwrites = {}
          sum_service_time =0 

          @readsInQ.times.map {Random.rand(CTMC.nbanks)}.map do |bank|
            bank_nreads[bank] =(bank_nreads[bank].nil?)?1:(bank_nreads[bank]+1)
          end
          @writesInQ.times.map {Random.rand(CTMC.nbanks)}.map do |bank|
            bank_nwrites[bank] =(bank_nwrites[bank].nil?)?1:(bank_nwrites[bank]+1)
          end
          #puts bank_nwrites
          #puts bank_nreads
          bus = Bus.new
          bank_nreads.each do |bank,reads|
            point = CTMC.rdServTime
            reads.times do
              bus.fill_first_empty_slot(point,CTMC.busContention)
              sum_service_time += point+CTMC.busContention
              point+=CTMC.busContention+CTMC.rdServTime
            end
            writes = bank_nwrites[bank]
            if(!writes.nil?)
              writes.times do
                bus.fill_first_empty_slot(point,CTMC.busContention)
                sum_service_time += point+CTMC.busContention
                point+=CTMC.busContention+CTMC.wrServTime
              end
            end

          end
          bank_nwrites      
          service_rates << (@readsInQ+@writesInQ).to_f / sum_service_time

      end
      avg_service_rate = service_rates.inject{ |sum, el| sum + el }.to_f / nsamples
      add_edge(State.new(@readsInQ-1,@writesInQ),avg_service_rate*@readsInQ.to_f/x) if(@readsInQ > 0)
      add_edge(State.new(@readsInQ,@writesInQ-1),avg_service_rate*@writesInQ.to_f/x) if(@writesInQ > 0)

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
      File.open(File.join(DIR,"dtmc#{CTMC.phase}.m"),"w") do |mf|
        mf.puts "I = #{@@mi};"
        mf.puts "J = #{@@mj};"
        mf.puts "V = #{@@mv};"
        mf.puts "P = sparse(I,J,V,#{@@nodes.size},#{@@nodes.size});"
        mf.puts "P = P - speye(#{@@nodes.size});"
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
      File.open("perf.out","a") do |pf|
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
          wrUtil+=Ans[i] if(state.writeInQ!=0)
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
f=File.open(File.join(DIR,"radix.in.1"),"r") do |input_file|
  input_file.each_line do |line|
    inputs = line.split(' ')
    CTMC.phase = inputs[0].to_i
    CTMC.rdJobCount=inputs[1].to_f.round
    CTMC.wrJobCount=inputs[2].to_f.round

    CTMC.rdServTime=inputs[3].to_f.round
    CTMC.wrServTime=inputs[4].to_f.round

    CTMC.rdThinkTime=inputs[5].to_f
    CTMC.wrThinkTime=inputs[6].to_f
    CTMC.busContention=12
    CTMC.nbanks=8

    #CTMC.queueSize=inputs[7].to_i
    CTMC.queueSize = 64

    CTMC.generate_chain
    #CTMC.dump_edges
    CTMC.dump_matlab_code
    system `matlab -nodesktop -nosplash -r "run('dtmc#{CTMC.phase}.m')" > /dev/null`
    CTMC.dump_perf_params
  end
end

