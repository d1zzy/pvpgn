--[[
	Copyright (C) 2014 HarpyWar (harpywar@gmail.com)
	
	This file is a part of the PvPGN Project http://pvpgn.pro
	Licensed under the same terms as Lua itself.
]]--


function command_stats(account, text)
	if config.ghost_dota_server and config.ghost and (account.clienttag == CLIENTTAG_WAR3XP) then
		return gh_command_stats(account, text)
	end

	return 1
end
