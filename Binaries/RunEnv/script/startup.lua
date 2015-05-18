function VeNet.OnBaseStart(server)
	base = server
	VeResourceManager.CreateGroup("table", { VeResourceManager.GetDir().."table"})
	VeResourceManager.SetDefaultGroupName("table")
	VeResourceManager.Load_S({
		"table,player_table.txt"
	})
	top_gear_prop = base.LoadEntity("top_gear")
	server.SetTimeTick(top_gear_prop.time)
	server.ReadyForConnections()
end

function VeNet.OnBaseUpdate()
	top_gear_prop.time = base.GetTimeTick()
	base.SyncEntities()
	collectgarbage()
end
