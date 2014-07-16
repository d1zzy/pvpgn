--[[
	Copyright (C) 2014 HarpyWar (harpywar@gmail.com)
	
	This file is a part of the PvPGN Project http://pvpgn.pro
	Licensed under the same terms as Lua itself.
]]--


-- this function executes after preload all the lua scripts
function main()
	
	-- start antihack
	if (config.ah) then
		ah_init()
	end

end
