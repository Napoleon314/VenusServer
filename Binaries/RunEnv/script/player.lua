player = {}

--	<CODE-USER-AREA>[user_area]

--	</CODE-USER-AREA>[user_area]

function player:CreateRole(name,role)
--	<CODE-FUNC-IMPLEMENT>{CreateRole}
	if role < 3 then
		self.prop.state = "account_normal"
		self.prop.info = {0,128,256,32,name,205010,64,102505,120}
		self.prop.card.push_back({1000*(role+1)+1,7,3})
		if role == 0 then
			self.prop.card.push_back({2001,6,2})
			self.prop.card.push_back({3001,6,2})
		end
		if role == 1 then
			self.prop.card.push_back({1001,6,2})
			self.prop.card.push_back({3001,6,2})
		end
		if role == 2 then
			self.prop.card.push_back({1001,6,2})
			self.prop.card.push_back({2001,6,2})
		end
		self.prop.card.push_back({4001,6,2})
		self.prop.card.push_back({5001,6,2})
		self.prop.chip.push_back({1001,240})
		self.prop.chip.push_back({2001,240})
		self.prop.chip.push_back({3001,240})
		self.prop.chip.push_back({4001,240})
		self.prop.chip.push_back({5001,240})
		CreateData(self.prop)
		return "true"
	else
		return "false"
	end
--	</CODE-FUNC-IMPLEMENT>{CreateRole}
end

function player:LeaderSwap(index)
--	<CODE-FUNC-IMPLEMENT>{LeaderSwap}
	local card = self.prop.card
	if index > 0 and index < card.size then
		local temp = {card[0].id,card[0].level,card[0].quality}
		card[index + 1],card[0],card[1] = temp,card[index + 1],card[index + 1]
	end	
--	</CODE-FUNC-IMPLEMENT>{LeaderSwap}
end

function player:CardLevelUp(index,gold_price,exp_price)
--	<CODE-FUNC-IMPLEMENT>{CardLevelUp}
	local card = self.prop.card
	local level_pass = card[index].level < self.prop.info.level
	local enough_gold = self.prop.info.gold >= gold_price
	local enough_exp = self.prop.info.exp_power >= exp_price
	if level_pass and enough_gold and enough_exp then
		card[index].level = card[index].level + 1
		self.prop.info.gold = self.prop.info.gold - gold_price
		self.prop.info.exp_power = self.prop.info.exp_power - exp_price
	end
--	</CODE-FUNC-IMPLEMENT>{CardLevelUp}
end

function player:CardQualityUp(index,chip_price,gold_price)
--	<CODE-FUNC-IMPLEMENT>{CardQualityUp}
	local card = self.prop.card
	local quality_up = card[index].quality < 5
	local enough_chip = self.prop.chip[index].amount >= chip_price
	local enough_gold = self.prop.info.gold >= gold_price
	if quality_up and enough_chip and enough_gold then
		card[index].quality = card[index].quality + 1
		self.prop.info.gold = self.prop.info.gold - gold_price
		self.prop.chip[index].amount = self.prop.chip[index].amount - chip_price
	end
--	</CODE-FUNC-IMPLEMENT>{CardQualityUp}
end

function player:WarTrophies(gold,exp_power,chip)
--	<CODE-FUNC-IMPLEMENT>{WarTrophies}
	self.prop.info.gold = self.prop.info.gold + gold
	self.prop.info.exp_power = self.prop.info.exp_power + exp_power
	local chips = self.prop.chip
	local i
	local j
	for i in ipairs(chip) do
		for j = 1,chips.size,1 do
			if chips[j].id == chip[i].id then
				chips[j].amount = chips[j].amount + chip[i].amount
			end
		end
	end
--	</CODE-FUNC-IMPLEMENT>{WarTrophies}
end
