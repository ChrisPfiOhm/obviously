-- This is an example of using Lua statemachines
function doEntry ()
   print "--> enter Po state"
end

function doActive ()
   print " Po"
   return 1
end

function doExit ()
   print "<-- exit Po state"
   print ""
end
