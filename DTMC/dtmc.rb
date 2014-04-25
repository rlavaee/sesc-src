#require 'graphviz'

module DTMC
  RdJobCount = ARGV[0].to_i
  WrJobCount = ARGV[1].to_i

  RdServTime = ARGV[2].to_i
  WrServTime = ARGV[3].to_i

  RdThinkTime = ARGV[4].to_i
  WrThinkTime = ARGV[5].to_i

  QueueSize = ARGV[6].to_i


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
  
    @@nodes = Array.new
    @@nodeIndices = Hash.new

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

    def needs_sched
      return ((@residual==0) and (@readsInQ!=0 or @writesInQ!=0))
    end

		def normalize_edges
			fout = @out_edges.inject(0) { |result,oe| result+oe.rate}
			@out_edges.each {|edge| edge.rate/=fout}
		end

		def State.normalize_all_edges
			@@nodes.each {|state| state.normalize_edges}
		end

    def add_edges
      if(needs_sched) #scheduling the next job in the queue
        x = @readsInQ+@writesInQ-1
        if(@serving=="read")
          add_edge(State.new(@readsInQ-1,@writesInQ,"read",RdServTime),(@readsInQ-1).to_f/x) if(@readsInQ > 1)
          add_edge(State.new(@readsInQ-1,@writesInQ,"write",WrServTime),@writesInQ.to_f/x) if(@writesInQ > 0)
					add_edge(State.new(0,0,"*",0),1) if(@readsInQ==1 and @writesInQ==0)
        elsif(@serving=="write")
          add_edge(State.new(@readsInQ,@writesInQ-1,"read",RdServTime),@readsInQ.to_f/x) if(@readsInQ > 0)
          add_edge(State.new(@readsInQ,@writesInQ-1,"write",WrServTime),(@writesInQ-1).to_f/x) if(@writesInQ > 1)
					add_edge(State.new(0,0,"*",0),1) if(@readsInQ==0 and @writesInQ==1)
        elsif(@serving=="*")
          add_edge(State.new(@readsInQ,@writesInQ,"read",RdServTime),1) if(@readsInQ > 0)
          add_edge(State.new(@readsInQ,@writesInQ,"write",WrServTime),1) if(@writesInQ > 0)
        end
      else
        rdArrivalRate = (RdJobCount-@readsInQ).to_f/RdThinkTime
        wrArrivalRate = (WrJobCount-@writesInQ).to_f/WrThinkTime
        noArrivalRate = 1 - rdArrivalRate - wrArrivalRate

        next_residual = (@serving=="*")?(@residual):(@residual-1)

        if(@readsInQ+@writesInQ < QueueSize)
          add_edge(State.new(@readsInQ+1,@writesInQ,@serving,next_residual), rdArrivalRate) if(@readsInQ < RdJobCount)
          add_edge(State.new(@readsInQ,@writesInQ+1,@serving,next_residual), wrArrivalRate) if(@writesInQ < WrJobCount)
        end
        add_edge(State.new(@readsInQ,@writesInQ,@serving,next_residual), noArrivalRate) 
      end

    end

    def dump_matlab_code
      #sum the flow out
      fout = @out_edges.inject(0) { |result,oe| result+oe.rate}
			raise "For #{self}, flow out is not equal to one, it's #{fout}" if(fout!=1)
      @in_edges.each do |ie|
				@@mi << @@nodeIndices[ie.to]+1
				@@mj << @@nodeIndices[ie.from]+1
				@@mv << ie.rate
      end

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
			puts "I = #{@@mi};"
			puts "J = #{@@mj};"
			puts "V = #{@@mv};"
			puts "P = sparse(I,J,V,#{@@nodes.size},#{@@nodes.size});"
			puts "P = P - speye(#{@@nodes.size});"
      puts "Y = sparse([zeros(#{@@nodes.size-1},1); 1]);"
      puts "X = P\\Y;"
    end


    def dump_edges  
      puts "#{self}:"
      @out_edges.each {|edge| puts edge}
    end

    def to_s
      "(#{@readsInQ},#{@writesInQ},#{@serving},#{@residual})"
    end
  
  end

  #the initial states
  State.add_node(0,0,"*",0)
   
  #(0..[RdJobCount,QueueSize].min ).each do |rds|
  #  (0..[QueueSize-rds,WrJobCount].min).each do |wrs|
  #    (0..RdServTime).each { |res| State.add_node(rds,wrs,"read",res) if(rds>0) }
  #    (0..WrServTime).each { |res| State.add_node(rds,wrs,"write",res) if(wrs>0) }  
  #  end
  #end


  State.add_all_edges
	State.normalize_all_edges
  #State.dump_all_edges

  State.dump_all_matlab_code
 
  #dtmc = GraphViz.new(:G, :type => :digraph)
  #dtmc.add_edges(emptyQ,st_1_1_2,"weight" => "30")
  #dtmc.add_edges(st_1_1_2,emptyQ,"weight" => "3")
  #dtmc.output(:pdf => "dtmc.pdf")

end
