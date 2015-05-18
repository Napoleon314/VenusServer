top_gear = {}

--	<CODE-USER-AREA>[user_area]
function CreateData(prop)
	if prop.state == "account_normal" then
		local temp = VeTextTable.GetRow("player_table.txt",prop.info.level)
		prop.data = {temp.exp_max,temp.buddy_list,temp.physical_power}
		prop.mission.push_back({"single",temp.single})
		prop.mission.push_back({"online",temp.online})
	end
end
--	</CODE-USER-AREA>[user_area]

function top_gear:Login(key,lan,verMaj,verMin)
--	<CODE-FUNC-IMPLEMENT>{Login}
	local player = base.LoadEntity("player", key)
	player.lan = lan
	if self.agent.LinkEntity(player) ~= nil then
		CreateData(player)
		return "login_succeed"
	else
		return "login_failed"
	end
--	</CODE-FUNC-IMPLEMENT>{Login}
end
