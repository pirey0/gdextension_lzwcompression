extends Node

var lzw := preload("res://gdgifexporter/gif-lzw/lzw.gd").new()

# Called when the node enters the scene tree for the first time.
func _ready():
	print("Start")
	
	var data_in = PackedByteArray()
	for x in 50000:
		data_in.append(randi_range(0,12))
	
	var color_table = []
	for i in range(12):
		color_table.append(i)
	
	var t0 = Time.get_ticks_usec()
	var out_gd = lzw.compress_lzw(data_in,color_table)
	print("OutGD Time: ", Time.get_ticks_usec() - t0)
	
	t0 = Time.get_ticks_usec()
	var out_c = LZWExtension.compress(data_in,color_table)
	print("OutC  Time: ", Time.get_ticks_usec() - t0)
	
	var mismatch := false
	for x in min(out_gd[0].size(), out_c.size()):
		if out_gd[0][x] != out_c[x]:
			mismatch = true
		if mismatch:
			prints(x, ":",out_gd[0][x],out_c[x])
	
	print("OutGD: " , out_gd[0].size())
	print("Out C: ", out_c.size())
	print("Equals:", [out_gd[0]].hash() == [out_c].hash())
	print("End")
