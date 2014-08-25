-- Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local utils
utils = {}

--
-- numbered_keys: transform an iterator to one that returns numbered keys
--
-- for k,v in numbered_keys(pairs(table)) do ... end
--
function utils.numbered_keys(step, context, position)
  local k = position
  local f = function(s, i)
    local v
    k,v = step(s, k)
    if k ~= nil then
      return (i+1), v
    end
  end
  return f, context, 0
end

--
-- filter: transform an iterator to one that only returns items that
--         match some predicate
--
-- for k,v in filter(function (k,v) ... return true end, pairs(table))
--
function utils.filter(predicate, step, context, position)
  local f = function (s, k)
    local v
    repeat k,v = step(s,k); until (k == nil) or predicate(k,v)
    return k,v
  end
  return f, context, position
end

--
-- map: transform an iterator to one that returns modified keys/values
--
-- for k,v in map(function (k,v) ... return newk, newv end, pairs(table))
--
function utils.map(transformer, step, context, position)
  local f = function (s, k)
    local v
    k, v = step(s, k)
    if k ~= nil then
      return transformer(k,v)
    end
  end
  return f, context, position
end

--
-- build_array: return a table containing all values returned by an iterator
--              returned table is built using table.insert (integer keys)
--
-- array = build_array(pairs(table))
--
function utils.build_array(f, s, k)
  local v
  local t = {}
  while true do
    k, v = f(s, k)
    if k == nil then break end
    table.insert(t, v)
  end
  return t
end

--
-- build_table: return a table containing all values returned by an iterator
--              returned table is build using t[k] = v
--
-- filtered = build_table(filter(function () ... end, pairs(table)))
--
function utils.build_table(f, s, k)
  local v
  local t = {}
  while true do
    k, v = f(s, k)
    if k == nil then break end
    t[k] = v
  end
  return t
end

--
-- stable_sort: return a sorted table. Sort isn't fast but stable
-- (default Lua table.sort is fast and unstable).
-- stable_sort uses Merge sort algorithm.
--
-- sorted_table = stable_sort(unsorted_table, 
--							  function (a,b) return a < b end)
--

function utils.stable_sort(values, cmp)
	if not cmp then 
		cmp = function (a,b) return a <= b end
	end
	
	local split = function (values) 
	   local a = {} 
	   local b = {} 
	   local len = #values 
	   local mid = math.floor(len/2)
	   for i = 1, mid do 
		  a[i] = values[i]
	   end 
	   for i = mid+1, len do 
		  b[i-mid] = values[i]
	   end 
	   return a,b
	end 

	local merge = function (a,b) 
	   local result = {}
	   local a_len = #(a)
	   local b_len = #(b)
	   local i1 = 1
	   local i2 = 1
	   for j = 1, a_len+b_len do 
		  if i2 > b_len 
			 or (i1 <= a_len and cmp(a[i1], b[i2]))
		  then
			 result[j] = a[i1]
			 i1 = i1 + 1
		  else                      
			 result[j] = b[i2]
			 i2 = i2 + 1
		  end
	   end 
	   return result 
	end 

	function merge_sort (values) 
	   if #values > 1 then
		  local a, b = split(values)
		  a = merge_sort(a)
		  b = merge_sort(b)
		  values = merge(a, b)
	   end
	   return values
	end
	
	return merge_sort(values)
end

--
-- split: split a string using a seperator char
--
-- returns: an array of strings
--
function utils.split(inputstr, sep)
        if sep == nil then
                sep = "%s"
        end
        t={} ; i=1
        for str in string.gmatch(inputstr, "([^"..sep.."]+)") do
                t[i] = str
                i = i + 1
        end
        return t, i
end

--
-- inspectObject: inspects and prints a description of an object
--
--
local inspectObject = function (title, object)
	-- Debug inspection
	object_dump = inspect(object)
	print("START of Serialization")
	print(title.." serlizable data character count: "..object_dump:len())
	object_dump_array, split_count = utils.split(object_dump, '}')
	if split_count > 1 then
		for key, val in pairs(object_dump_array) do
			print(val.."}")
		end
	else
		print(val)
	end
	print("END of Serialization")
end

return utils
