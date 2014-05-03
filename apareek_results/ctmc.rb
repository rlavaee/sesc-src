require 'priority_queue'
require 'fileutils'

module CTMC

	class BusRequest < Range
		attr_accessor :type
		attr_accessor :rank
		def initialize(a,b,type,rank,bank)
			super(a,b)
			@type = type
			@rank = rank
			@bank = bank
		end
		
		def to_s
			return "#{super}[#{@type},#{@rank},#{@bank}]"
		end

		def inspect
			return "#{super}[#{@type},#{@rank},#{@bank}]"
		end

	end

  class Bus
		attr_accessor :ranges
		attr_accessor :last_rb_comp
    def initialize(nranks,nbanks,servTime)
      @ranges = Array.new
			@last_rb_comp = PriorityQueue.new
			@almost_last_rb_comp_index = Hash.new
			(0..nranks-1).each do |rank|
				(0..nbanks-1).each do |bank|
				 @last_rb_comp[[rank,bank]]=0
				 @almost_last_rb_comp_index[[rank,bank]]=-1
				end
			end
			@servTime = servTime
    end

		def last_completion
			(@ranges.empty?)?(0):(@ranges.last.last)
		end


    def fill_first_empty_slot(length_before,length_after,type,rank,bank)
			point = @last_rb_comp[[rank,bank]]+@servTime[type]
      from_index = @almost_last_rb_comp_index[[rank,bank]]
			from = (from_index==-1)?(0):(@ranges[from_index])
			#puts "from index: #{from_index}"

			(from_index+1..@ranges.size-1).each do |i|
				range = @ranges[i]
				#puts i
        to = range.first

        if( (to-length_before < point+length_after) or ((to < point+length_after) and (range.type=="read" or (type=="write" and range.rank==rank))))
          from = range.last
 					if((from+length_before > point) and (type=="write" and (range.type=="read" or range.rank!=rank)))
           	point = from+length_before
        	elsif(from > point)
						point = from
					end
        else
					@ranges.insert(i,BusRequest.new(point,point+length_after,type,rank,bank))
					@last_rb_comp[[rank,bank]]=point+length_after
					@almost_last_rb_comp_index[[rank,bank]]=i
					return
				end

      end
			
      @ranges <<  BusRequest.new(point,point+length_after,type,rank,bank)
			@last_rb_comp[[rank,bank]]=point+length_after
			@almost_last_rb_comp_index[[rank,bank]]=@ranges.size-1
			return

    end

		def to_s
			@ranges.to_s
		end

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
    attr_reader :jobsInQs
		attr_reader :total_jobs
		attr_accessor :type_total_jobs

    def initialize(jobsInQs,total_jobs,type_total_jobs)
      @jobsInQs = jobsInQs
			@total_jobs = total_jobs
			@type_total_jobs = type_total_jobs
      @in_edges = []
      @out_edges = []
    end

    def State.add_node(jobsInQs,total_jobs,type_total_jobs)
			#puts @@nodes.size
      state = State.new(jobsInQs,total_jobs,type_total_jobs)
      get_stored_node(state)
    end

    def State.get_stored_node(state)
      if(!@@nodeIndices.include?(state))
        #puts "adding node #{state}"
        @@nodeIndices[state]= @@nodes.size
        @@nodes << state
				if( @@nodes.size>2*@@prev)
					@@prev = @@nodes.size
					puts @@prev
				end
        return state
      else
        return @@nodes[@@nodeIndices[state]]
      end
    end

    #Hash needs this
    def eql?(state)
			@jobsInQs.eql?(state.jobsInQs)
    end

    #Hash needs this too
    def hash
			@jobsInQs.hash
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

		def distribute_jobs (njobs)
			jobs = Hash.new(0)
			njobs.times.map {[Random.rand(@@nranks),Random.rand(@@nbanks)]}.each do |rands|
				jobs[rands]+=1
			end
			return jobs
		end

		def State.job_type_map (init_proc)
			return @@jobTypes.inject({}) {|res,job_type| res[job_type]=init_proc.call(job_type); res}
		end

    def add_service_edges
      #puts "adding service edges for #{self}"
			(0..@@nchannels-1).each do |channel|
				service_rates = State.job_type_map(Proc.new{|jt| Array.new})
      	@@nsamples.times.each do |sample|
		
					jobs_in_ch = State.job_type_map(Proc.new{|jt| distribute_jobs(@jobsInQs[jt][channel])})

          bus = Bus.new(@@nranks,@@nbanks,@@servTime)


					while(!bus.last_rb_comp.empty?) do
						rb = bus.last_rb_comp.min.first
						rank = rb[0]
						bank = rb[1]

						sum_jobs=jobs_in_ch.inject(0) {|sum,(jt,val)| sum+val[rb]}
						if(sum_jobs!=0)
							rand = Random.rand(sum_jobs)
							cum_jobs = jobs_in_ch.inject(0) do |sum,(jt,val)|
								new_sum = sum+val[rb]
								if(rand<new_sum)
									point=bus.fill_first_empty_slot(@@rankToRank,@@burstLength,jt,rank,bank)
									jobs_in_ch[jt][rb]-=1
									break
								end
								new_sum
							end
            else
							bus.last_rb_comp.delete(rb)
						end

					end
						
					#puts bus.to_s
					#puts "\n"
					last = bus.last_completion
					service_rates.each do |job_type,rates|
						service_rates[job_type] << @jobsInQs[job_type][channel].to_f / last if (last!=0)
					end
				end

      	service_rates.each do |job_type,rates|
					if(@jobsInQs[job_type][channel] > 0)
						avg_service_rate = rates.inject(0) { |sum, el| sum + el }.to_f / @@nsamples
						newJobsInQs = clone_jobs_in_qs
						newJobsInQs[job_type][channel]-=1
      			add_edge(State.add_node(newJobsInQs,@total_jobs-1,State.job_type_map(Proc.new{|jt| (jt==job_type)?(@type_total_jobs[jt]-1):(@type_total_jobs[jt])})),avg_service_rate)
					end
				end

    	end
		end

		def clone_jobs_in_qs
			State.job_type_map(Proc.new {|jt| @jobsInQs[jt].clone})
		end

    def add_edges
          add_service_edges
					if(@total_jobs < @@queueSize)
						@@jobTypes.each do |job_type|
							(0..@@nchannels-1).each do |channel|
								if(@type_total_jobs[job_type] < @@jobCount[job_type])
									arrival_rate = (@@jobCount[job_type]-@type_total_jobs[job_type]).to_f/@@thinkTime[job_type]
									newJobsInQs = clone_jobs_in_qs
									newJobsInQs[job_type][channel]+=1
									add_edge(State.add_node(newJobsInQs,@total_jobs+1,State.job_type_map(Proc.new{|jt| (jt==job_type)?(@type_total_jobs[jt]+1):(@type_total_jobs[jt])})),arrival_rate)
								end
							end
						end
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
			@@ans = nil
			@@prev = 0
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
      File.open(File.join(@@dir,"ctmc#{@@phase}.m"),"w") do |mf|
        mf.puts "I = #{@@mi};"
        mf.puts "J = #{@@mj};"
        mf.puts "V = #{@@mv};"
        mf.puts "P = sparse(I,J,V,#{@@nodes.size},#{@@nodes.size});"
        #mf.puts "P = P - speye(#{@@nodes.size});"
        mf.puts "Y = sparse([zeros(#{@@nodes.size-1},1); 1]);"
        mf.puts "X = P\\Y;"
        mf.puts "fid = fopen('answer#{@@phase}.txt','w');"
        mf.puts "s = repmat('%f,',1,length(X));"
        mf.puts "s(end)=[]; %Remove trailing comma"
        mf.puts "disp(fprintf(fid,['@@ans = [' s ']'], full(X)));"
        mf.puts "exit"
      end
    end

		def State.get_ctmc_ans
    	f = File.open(File.join(@@dir,"answer#{@@phase}.txt"),"r") do |f|
      	text = f.read
      	eval(text.lines.first)
    	end
			
      
  	end


    def State.dump_perf_params
			puts "sum is #{@@ans.inject(0) {|sum,val| sum+val}}"
      File.open(@@output,"a") do |pf|
        pf.write "#{@@phase}\t"
				thru = State.job_type_map(Proc.new{|jt| 0})
				util = State.job_type_map(Proc.new{|jt| 0})
				qocc = State.job_type_map(Proc.new{|jt| 0})
				respt = State.job_type_map(Proc.new{|jt| 0})

				@@jobTypes.each do |job_type|
					(0..@@nchannels-1).each do |ch|
						@@nodes.each_with_index do |state,i|
							sum_jobs = @@jobTypes.inject(0) {|sum,jt| sum+state.jobsInQs[jt][ch]}
              qocc[job_type] += @@ans[i]*state.jobsInQs[job_type][ch]
							util[job_type] += @@ans[i]*(state.jobsInQs[job_type][ch].to_f / sum_jobs) if(sum_jobs!=0)
            	state.out_edges.each do |edge|
              	thru[job_type] += @@ans[i]*edge.rate if(edge.to.jobsInQs[job_type][ch] > edge.from.jobsInQs[job_type][ch])
							end
            end
        	end
					qocc[job_type]/=@@nchannels
					thru[job_type]/=@@nchannels
					util[job_type]/=@@nchannels
				end

				State.job_type_map (Proc.new {|jt| pf.write("#{thru[jt]}\t")})
				State.job_type_map (Proc.new {|jt| pf.write("#{util[jt]}\t")})
				State.job_type_map (Proc.new {|jt| pf.write("#{qocc[jt]}\t")})
				State.job_type_map (Proc.new {|jt| pf.write("#{qocc[jt]/thru[jt]}\t")})
				pf.write "\n"

      end
    end


    def dump_edges  
			puts "#{self}:"
      @out_edges.each {|edge| puts edge}
    end

    def to_s
      @jobsInQs.inspect
    end

		def State.generate_chain
    	State.initialize_all
			init_queue = []
    	init = State.add_node(State.job_type_map(Proc.new {|jt| Array.new(@@nchannels,0)}),0,job_type_map(Proc.new{|jt| 0}))
			State.add_all_edges
			#State.normalize_all_edges
  	end

		def State.output_header
			File.open(@@output,"w") do |pf|
				pf.write("phase\t")
				job_type_map (Proc.new {|jt| pf.write("#{jt}_thru\t")})
				job_type_map (Proc.new {|jt| pf.write("#{jt}_util\t")})
				job_type_map (Proc.new {|jt| pf.write("#{jt}_qocc\t")})
				job_type_map (Proc.new {|jt| pf.write("#{jt}_respt\t")})
				pf.write "\n"
			end

		end

  end


  
end


include CTMC
class State
	@@input = ARGV[0]
	extname = File.extname(ARGV[0])
	basename = File.basename(ARGV[0],extname)
	@@dir = File.join(File.dirname(ARGV[0]),basename)
	FileUtils.mkdir(@@dir) if !Dir.exists?(@@dir)
	@@output = File.join(@@dir,"output-ctmc"+extname)
	@@nchannels=ARGV[1].to_i
	@@nranks = ARGV[2].to_i
	@@nbanks = ARGV[3].to_i
	@@burstLength = 4
	@@rankToRank = 6
  @@queueSize = (@nchannels==1)?(64):(40)
	@@nsamples = (@nchannels==1)?(30):(5)
	@@jobTypes = ["read","write"]
	@@jobCount = {}
	@@servTime = {}
	@@thinkTime = {}
	@@prev = 0

	@@jobCount = job_type_map (Proc.new {|jt| 0})
	@@servTime = job_type_map (Proc.new {|jt| 0})
	@@thinkTime = job_type_map (Proc.new {|jt| 0})

	State.output_header


	@@phase = 1
	File.open(@@input,"r") do |infile|
				
  			infile.each_line do |line|
					puts line
					inputs = line.split(/[ \t]+/).to_enum
					@@jobTypes.each {|job_type| @@jobCount[job_type]=[inputs.next.to_f.round,1].max}
					@@jobTypes.each {|job_type| @@servTime[job_type]=inputs.next.to_f.round-@@burstLength}
					@@jobTypes.each {|job_type| @@thinkTime[job_type]=inputs.next.to_f}

		    	generate_chain
    			#dump_all_edges
    			dump_all_matlab_code
    			system `matlab -nodesktop -nosplash -r "run('#{File.join(@@dir,"ctmc#{@@phase}.m")}')" > /dev/null`
					get_ctmc_ans
    			dump_perf_params
					@@phase+=1
				end
  	end
end

