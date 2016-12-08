import sys
import numpy as np

import tensorflow as tf

def _float_feature(value):
	return tf.train.Feature(float_list=tf.train.FloatList(value=[value]))

def _int64_feature(value):
	return tf.train.Feature(int64_list=tf.train.Int64List(value=[value]))

def _bytes_feature(value):
	return tf.train.Feature(bytes_list=tf.train.BytesList(value=[value]))

def load_module():

	mod = {'module_name': 'TFRecords'}

	param = [
		{'required': false, 'option': 'tfrecords-entries-per-shard', 'example': "  --tfrecords-entries-per-shard <num>", 'description': "Number of entries per shard. Set to 0 to disable sharding (default: 0)."},
		{'required': false, 'option': 'tfrecords-max-shard-size', 'example': "  --tfrecords-max-shard-size <bytes>", 'description': "Maximum size of a single shard. Once this limit is reach additiona shards are created (default: 0)."}
		]

	mod['param'] = param

	return mod

def initialize(options):
	global writer

	writer = {}
	print(options)

	if options["create_train_file"]:
		writer["train"] = tf.python_io.TFRecordWriter(options["working_dir"] + "train.tfr")
	if options["create_test_file"]:
		writer["test"] = tf.python_io.TFRecordWriter(options["working_dir"] + "test.tfr")
	if options["create_validate_file"]:
		writer["validate"] = tf.python_io.TFRecordWriter(options["working_dir"] + "validate.tfr")

	return True

def write_data(entry, source, ground):
	global writer

	if entry['type'] not in writer:
		print("'" + entry['type'] + "' is not opened as a writer type")
		return False

	# Currently we only support 1 source and 1 ground value
	if len(source) != 1 or len(ground) != 1:
		return False

	src = source[0]
	gnd = ground[0]

	record = {}
#	print(gnd)
	# Check the passed Source data and convert accordingly
	if src['value_type'] in ['image', 'raw']:

		# These apply to all image/raw types
		if 'channels' in src.values():
			record['source_depth'] = _int64_feature(src['channels'])
		if 'width' in src.values():
			record['source_width'] = _int64_feature(src['width'])
		if 'height' in src.values():
			record['source_height'] = _int64_feature(src['height'])

		if src['value_type'] == 'image':
			record['data'] = _bytes_feature(src['imagedata'].tobytes())
		elif src['value_type'] == 'raw':
			record['data'] = _bytes_feature(src['data'].tobytes())
	else:
		print("TFRecordsWriter does not support '" + src['value_type'] + "' as a Source value")
		return False

	# Check the passed Groundtruth data and convert accordingly
	if gnd['value_type'] in ['image', 'raw']:

		# These apply to all image/raw types
		if 'channels' in gnd.values():
			record['ground_depth'] = _int64_feature(gnd['channels'])
		if 'width' in gnd.values():
			record['ground_width'] = _int64_feature(gnd['width'])
		if 'height' in gnd.values():
			record['ground_height'] = _int64_feature(gnd['height'])
	
		if gnd['value_type'] == 'image':
			record['label'] = _bytes_feature(gnd['imagedata'].tobytes())
		elif gnd['value_type'] == 'raw':
			record['label'] = _bytes_feature(gnd['data'].tobytes())

	elif gnd['value_type'] == 'numeric':
		if gnd['dtype'] == "int":
			record['label'] = _int64_feature(gnd['value'])
		elif gnd['dtype'] == "float":
			record['label'] = _float_feature(gnd['value'])
		else:
			print("TFRecordsWriter does not support dtype '" + gnd['dtype'] + "' for value type '" + gnd['value_type'] + "'")
			return False
	else:
		print("TFRecordsWriter does not support '" + gnd['value_type'] + "' as a Ground value")
		return False

	example = tf.train.Example(features=tf.train.Features(feature=record))

	writer[entry['type']].write(example.SerializeToString())

	return True

def finalize():
	print("Script finalized")
	return True
