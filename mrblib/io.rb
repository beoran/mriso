##
# IO
#


# ISO 15.2.34
class IOError < StandardError
end

# ISO 15.2.35
class EOFError < IOError
end

# ISO 15.2.36
class SystemCallError < StandardError
end


# ISO 15.3.1
class IO
  def self.open(*args, &block) 
    i = self.new(*args)
    return i unless (block)
    begin 
    v = block.call(i)
    rescue Exception => ex
    ensure
      i.close
    end
    return v
  end
  
  
  # 15.2.20.5.11
  def print(*args) 
    nils = "nil"
    i = 0
    len = args.size
    while i < len
      arg = args[i]
      if args.nil?
        write(nils)
      else
        write(arg)
      end
      i += 1
    end
  end
  
  # 15.2.30.5.13 
  def puts(*args)
    len = args.size
    return write "\n" if len == 0 
    i = 0
    len = args.size
    while i < len
      arg = args[i]
      if arg.is_a? Array
        j = 0
        len = arg.size
        while j < len
          val = arg[j]
          if val == arg
            write("recursion in array...")
          else
            write(val)
          end
          j += 1
        end
      else 
        val = nil
        if arg.nil?
          val = "nil"
        elsif !(arg.is_a? String)
          val = arg.to_s
        else 
          val = arg
        end
        write val
        write "\n" unless val[-1] == "\n"
      end
      i += 1
    end
  end
  
  
  
end


