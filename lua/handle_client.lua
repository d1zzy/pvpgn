--[[
	Copyright (C) 2014 HarpyWar (harpywar@gmail.com)
	
	This file is a part of the PvPGN Project http://pvpgn.pro
	Licensed under the same terms as Lua itself.
]]--

-- Receive SID_READMEMORY
function handle_client_readmemory(account, request_id, data)
	
	--DEBUG("Read memory request Id: " .. request_id)
		
	-- display memory bytes
	--TRACE(data)
	
	if (config.ah) then
		ah_handle_client(account, request_id, data)
	end

end

-- Receive SID_EXTRAWORK
function handle_client_extrawork(account, gametype, length, data)
	--DEBUG(string.format("Received EXTRAWORK packet with GameType: %d and Length: %d (%s)", gametype, length, data))
	
end
