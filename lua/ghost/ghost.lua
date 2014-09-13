--[[
	Copyright (C) 2014 HarpyWar (harpywar@gmail.com)
	
	This file is a part of the PvPGN Project http://pvpgn.pro
	Licensed under the same terms as Lua itself.
]]--

-- custom map list
gh_maplist = {}

function gh_load()
	-- load ghost state
	local filename = config.vardir() .. "ghost_users.dmp"
	if file_exists(filename) then
		gh_load_userbots(filename)
		DEBUG("Loaded GHost state from " .. filename)
	end
	
	-- preload custom map list
	local mapfile = gh_directory() .. "/maplist.txt"
	if file_exists(mapfile) then
		gh_load_maps(mapfile)
		DEBUG("Loaded " .. #gh_maplist .. " GHost custom maps")
	else
		WARN("File with GHost custom map list is not found: " .. mapfile)
	end	

end

function gh_unload()
	-- save ghost state
	local filename = config.vardir() .. "ghost_users.dmp"
	gh_save_userbots(filename)
	
	DEBUG("Saved GHost state to " .. filename)
end




function gh_load_maps(filename)
	-- load maps from the file
	file_load(filename, file_load_dictionary_callback, 
		function(a,b)
			-- split second part of the line to get mapname and filename
			local idx = 0
			for v in string.split(b, "|") do
				if idx == 0 then
					mapname = string:trim(v)
				else
					mapfile = string:trim(v)
				end
				idx = idx + 1
			end
			table.insert(gh_maplist, { code = a, name = mapname, filename = mapfile })
		end)
end