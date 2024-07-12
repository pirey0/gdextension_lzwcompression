extends Node

var lzw := preload("res://gdgifexporter/gif-lzw/lzw.gd").new()

const GIFExporter = preload("res://gdgifexporter/exporter.gd")
# load quantization module that you want to use
const MedianCutQuantization = preload("res://gdgifexporter/quantization/median_cut.gd")

# Called when the node enters the scene tree for the first time.
func _ready():
	export_test()

func export_test():
	var t0 = Time.get_ticks_usec()
	export_gif(false)
	print("OutGD Time: ", Time.get_ticks_usec() - t0)
	t0 = Time.get_ticks_usec()
	export_gif(true)
	print("OutC Time: ", Time.get_ticks_usec() - t0)

func export_gif(c):
	var img := Image.new()
	# load your image from png file
	img.load("res://test.png")
	# remember to use this image format when exporting
	img.convert(Image.FORMAT_RGBA8)

	# initialize exporter object with width and height of gif canvas
	var exporter = GIFExporter.new(img.get_width(), img.get_height())
	exporter.lzw.run_c_converter = c
	
	# write image using median cut quantization method and with one second animation delay
	exporter.add_frame(img, 1, MedianCutQuantization)

	img.load("res://test2.jpg")
	img.convert(Image.FORMAT_RGBA8)
	exporter.add_frame(img, 1, MedianCutQuantization)
	
	# when you have exported all frames of animation you, then you can save data into file
	# open new file with write privlige
	var path = "user://result" + ("_c" if c else "_gd") + ".gif"
	var file: FileAccess = FileAccess.open(path, FileAccess.WRITE)
	print(ProjectSettings.globalize_path(path))
	# save data stream into file
	file.store_buffer(exporter.export_file_data())
	# close the file
	file.close()
	pass

func compare_algo(al:Callable, name_l:String,ar:Callable, name_r:String):
	print("Start")
	
	var data_in = PackedByteArray()
	for x in 100000:
		data_in.append(randi_range(0,12))
	
	var color_table = []
	for i in range(12):
		color_table.append(i)
	
	var t0 = Time.get_ticks_usec()
	var out_gd = al.call(data_in,color_table)
	var tl = Time.get_ticks_usec() - t0
	
	t0 = Time.get_ticks_usec()
	var out_c = ar.call(data_in,color_table)
	var tr = Time.get_ticks_usec() - t0
	
	print(name_l," Time: ", tl)
	print(name_r + " Time: ", tr)
	print(float(tr)/tl)
	
	var gdstream = out_gd["stream"]
	var cstream = out_c["stream"]
	
	var mismatch := false
	for x in min(gdstream.size(), cstream.size()):
		if gdstream[x] != cstream[x]:
			mismatch = true
		if mismatch:
			prints(x, ":",gdstream[x],cstream[x])
	
	print("Equals:", [gdstream].hash() == [cstream].hash() and out_gd["min_code_size"] == out_c["min_code_size"])
	print("End")

func run_test():
	compare_algo(
			lzw.compress_lzw,
		 "GDScript Original",
		func(a,b): return FastLZWCompressor.compress(a,b),
		 "C Fast Rewrite")
	pass
