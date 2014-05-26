--[[
	Copyright (C) 2014 HarpyWar (harpywar@gmail.com)
	
	This file is a part of the PvPGN Project http://pvpgn.pro
	Licensed under the same terms as Lua itself.
]]--


-- Get count of all games 
function games_get_count()
	local count = 0
	for id,gamename in pairs(api.server_get_games()) do
		count = count + 1
	end
	return count
end

