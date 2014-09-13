--[[
	Copyright (C) 2014 HarpyWar (harpywar@gmail.com)
	
	This file is a part of the PvPGN Project http://pvpgn.pro
	Licensed under the same terms as Lua itself.
]]--


function command_ping(account, text)
	-- allow warcraft 3 client only
	if (account.clienttag == CLIENTTAG_WAR3XP) and (config.ghost) then
		return gh_command_ping(account, text)
	end

	return 1
end
