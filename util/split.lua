#!/home/graehl/torch/install/bin/luajit

local function grjoined(tokens)
    return table.concat(tokens, ' ')
end

local function grsplit(str, sSeparator, nMax, bRegexp)
   sSeparator = sSeparator or ' '
   assert(sSeparator ~= "")
   assert(nMax == nil or nMax >= 1)
   local aRecord = {}
   if str:len() > 0 then
      local bPlain = not bRegexp
      nMax = nMax or -1
      local nStart = 1, 1
      local nFirst, nLast = str:find(sSeparator, nStart, bPlain)
      while nFirst and nMax ~= 0 do
         table.insert(aRecord, str:sub(nStart, nFirst-1))
         nStart = nLast + 1
         nFirst, nLast = str:find(sSeparator, nStart, bPlain)
         nMax = nMax - 1
      end
      table.insert(aRecord, str:sub(nStart))
   end
   return aRecord
end

local function deBPE(tokens, bpecont)
    return grsplit(string.gsub(table.concat(tokens, ' '), bpecont .. ' ', ''), ' ')
end
