#require 'graphviz'

module DTMC
  class << self
    attr_accessor :phase
    attr_accessor :rdJobCount
    attr_accessor :wrJobCount
    attr_accessor :rdServTime
    attr_accessor :wrServTime
    attr_accessor :rdThinkTime
    attr_accessor :wrThinkTime
    attr_accessor :queueSize
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
    attr_reader :serving
    attr_reader :residual

    def initialize(readsInQ=0,writesInQ=0,serving=0,residual=0)
      @readsInQ = readsInQ
      @writesInQ = writesInQ
      @residual = residual
      @serving = serving
      @in_edges = []
      @out_edges = []
    end

    def State.add_node(rds,wrs,serving,res)
      state = State.new(rds,wrs,serving,res)
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
      (@readsInQ == state.readsInQ) and (@writesInQ == state.writesInQ) and (@residual == state.residual) and (@serving == state.serving)
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

    def is_init
      return (@serving=="*")
    end

    def normalize_edges
      fout = @out_edges.inject(0) { |result,oe| result+oe.rate}
      @out_edges.each {|edge| edge.rate/=fout}
    end

    def State.normalize_all_edges
      @@nodes.each {|state| state.normalize_edges}
    end

    def add_arrival_edges(nextReadsInQ,nextWritesInQ,nextServing,nextResidual,norm)
      rdArrivalRate = (DTMC.rdJobCount-@readsInQ).to_f/DTMC.rdThinkTime
      wrArrivalRate = (DTMC.wrJobCount-@writesInQ).to_f/DTMC.wrThinkTime
      noArrivalRate = 1 - rdArrivalRate - wrArrivalRate


      if(nextServing=="*")
        add_edge(State.new(1,0,"read",DTMC.rdServTime), rdArrivalRate*norm)
        add_edge(State.new(0,1,"write",DTMC.wrServTime), wrArrivalRate*norm) 
        add_edge(State.new(0,1,"*",0), noArrivalRate*norm) 
      else
        if(@readsInQ+@writesInQ < DTMC.queueSize)
          add_edge(State.new(nextReadsInQ+1,nextWritesInQ,nextServing,nextResidual), rdArrivalRate*norm) if(@readsInQ < DTMC.rdJobCount)
          add_edge(State.new(nextReadsInQ,nextWritesInQ+1,nextServing,nextResidual), wrArrivalRate*norm) if(@writesInQ < DTMC.wrJobCount)
        end
        add_edge(State.new(nextReadsInQ,nextWritesInQ,nextServing,nextResidual), noArrivalRate*norm) 
      end
    end

    def add_edges
      if(is_init)
        add_arrival_edges(0,0,"*",0,1)
      elsif(@residual==1)
        x = @readsInQ+@writesInQ-1
        if(@serving=="read")
          add_arrival_edges(@readsInQ-1,@writesInQ,"read",DTMC.rdServTime,(@readsInQ-1).to_f/x) if(@readsInQ > 1)
          add_arrival_edges(@readsInQ-1,@writesInQ,"write",DTMC.wrServTime,@writesInQ.to_f/x) if(@writesInQ > 0)
          add_arrival_edges(@readsInQ-1,@writesInQ,"*",0,1) if(@writesInQ ==0 and @readsInQ ==1)
        elsif(@serving=="write")
          add_arrival_edges(@readsInQ,@writesInQ-1,"read",DTMC.rdServTime,@readsInQ.to_f/x) if(@readsInQ > 0)
          add_arrival_edges(@readsInQ,@writesInQ-1,"write",DTMC.wrServTime,(@writesInQ-1).to_f/x) if(@writesInQ > 1)
          add_arrival_edges(@readsInQ,@writesInQ-1,"*",0,1) if(@writesInQ ==1 and @readsInQ ==0)
        end
      else
        add_arrival_edges(@readsInQ,@writesInQ,@serving,@residual-1,1)
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
      File.open("dtmc#{DTMC.phase}.m","w") do |mf|
        mf.puts "I = #{@@mi};"
        mf.puts "J = #{@@mj};"
        mf.puts "V = #{@@mv};"
        mf.puts "P = sparse(I,J,V,#{@@nodes.size},#{@@nodes.size});"
        mf.puts "P = P - speye(#{@@nodes.size});"
        mf.puts "Y = sparse([zeros(#{@@nodes.size-1},1); 1]);"
        mf.puts "X = P\\Y;"
        mf.puts "fid = fopen('answer#{DTMC.phase}.txt','w');"
        mf.puts "s = repmat('%f,',1,length(X));"
        mf.puts "s(end)=[]; %Remove trailing comma"
        mf.puts "disp(fprintf(fid,['Ans = [' s ']'], full(X)));"
        mf.puts "exit"
      end
    end

    def State.dump_perf_params
      File.open("perf.out","a") do |pf|
        pf.write "#{DTMC.phase}\t"
        rdThru=0
        wrThru=0
        @@nodes.each_with_index do |state,i|
          if state.residual==1
            if state.serving == "read"
              rdThru += Ans[i]
            elsif state.serving == "write"
              wrThru += Ans[i]
            end
          end
        end
        pf.write "#{rdThru}\t"
        pf.write "#{wrThru}\t"

        rdUtil=0
        wrUtil=0
        @@nodes.each_with_index do |state,i|
          if state.serving == "read"
            rdUtil += Ans[i]
          elsif state.serving == "write"
            wrUtil += Ans[i]
          end
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
          if(state.readsInQ+state.writesInQ < DTMC.queueSize)
            rdArrival+=Ans[i]*(DTMC.rdJobCount-state.readsInQ)/ DTMC.rdThinkTime
            wrArrival+=Ans[i]*(DTMC.wrJobCount-state.writesInQ)/ DTMC.wrThinkTime
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
      "(#{@readsInQ},#{@writesInQ},#{@serving},#{@residual})"
    end

  end

  def generate_chain
    #the initial states
    State.initialize_all
    init = State.add_node(0,0,"*",0)


    State.add_all_edges
    State.normalize_all_edges
  end

  def dump_matlab_code
    State.dump_all_matlab_code
  end

  def dump_perf_params
    f = File.open("answer#{DTMC.phase}.txt","r") do |f|
      text = f.read
      eval(text.lines.first)
    end
    State.dump_perf_params
  end

  def dump_edges
    State.dump_all_edges
  end

end


include DTMC
f=File.open("radix.in","r") do |input_file|
  input_file.each_line do |line|
    inputs = line.split(' ')
    DTMC.phase = inputs[0].to_i
    DTMC.rdJobCount=inputs[1].to_f.round
    DTMC.wrJobCount=inputs[2].to_f.round

    DTMC.rdServTime=inputs[3].to_f.round
    DTMC.wrServTime=inputs[4].to_f.round

    DTMC.rdThinkTime=inputs[5].to_f
    DTMC.wrThinkTime=inputs[6].to_f

    DTMC.queueSize=inputs[7].to_i

    DTMC.generate_chain
    DTMC.dump_matlab_code
    system `matlab -nodesktop -nosplash -r "run('dtmc#{DTMC.phase}.m')" > /dev/null`
    DTMC.dump_perf_params
  end
end

