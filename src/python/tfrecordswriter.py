# Skip loading all libraries when just getting the module's arguments
if __name__ != "check_module":
	import sys
	import numpy as np
	import hashlib

	import tensorflow as tf
else:
	# Check if tensorflow exists on the system
	import imp
	imp.find_module('tensorflow')

cur_filename = None
file_itr = None

def _float_feature(value):
	return tf.train.Feature(float_list=tf.train.FloatList(value=[value]))

def _int64_feature(value):
	return tf.train.Feature(int64_list=tf.train.Int64List(value=[value]))

def _bytes_feature(value):
	return tf.train.Feature(bytes_list=tf.train.BytesList(value=[value]))

def load_module():

	mod = {'module_name': 'TFRecords'}

	param = [
				{'required': False, 'option': 'tfrecords-entries-per-shard', 'example': "  --tfrecords-entries-per-shard <num>", 'description': "Number of entries per shard. Set to 0 to disable sharding (default: 0)."},
				{'required': False, 'option': 'tfrecords-max-shard-size', 'example': "  --tfrecords-max-shard-size <bytes>", 'description': "Maximum size of a single shard. Once this limit is reached additional shards are created (default: 0)."}
			]

	mod['params'] = param

	return mod

def can_handle(feature):

	if feature == "resume":
		return True
	if feature == "blobs":
		return True
	if feature == "integrity":
		return True

	return False

def initialize(options, dataset_meta, mode):
	global writer
	global output_fields
	global g_options

	writer = {}
	output_fields = {}
	g_options = options

	for field in dataset_meta['fields']:
		output_fields[field['id']] = field['name']

	# Only create new TFR files if we are in MODE_NEW
	if mode == 0:
		for dataset in dataset_meta['sets']:
			writer[dataset] = tf.python_io.TFRecordWriter(options["working_dir"] + dataset + ".tfr")

	return True

def begin_writing():
	return True

def write_data(entry):
	global writer
	global output_fields

	record = {}

	# Check the passed Source data and convert accordingly
	for field in entry['entries']:
		out_field = output_fields[field['id']]

		if field['value_type'] in ['image', 'raw']:
			if field['value_type'] == 'image':
				record[out_field] = _bytes_feature(field['imagedata'].tobytes())
			elif field['value_type'] == 'raw':
				record[out_field] = _bytes_feature(field['data'].tobytes())
		elif field['value_type'] == 'numeric':
			if field['dtype'] == "int":
				record[out_field] = _int64_feature(field['value'])
			elif field['dtype'] == "float":
				record[out_field] = _float_feature(field['value'])
			else:
				print("TFRecordsWriter does not support dtype '" + field['dtype'] + "' for value type '" + field['value_type'] + "'")
				return False
		elif field['value_type'] == 'string':
			record[out_field] = _bytes_feature(str.encode(field['value']))
		else:
			print("TFRecordsWriter does not support '" + field['value_type'] + "'")
			return False

	example = tf.train.Example(features=tf.train.Features(feature=record))
	data = example.SerializeToString()
	writer[entry['set']].write(data)

	m = hashlib.md5()
	m.update(data)

	return "file=" + entry['set'] + ".tfr;hash=" + m.hexdigest()

def check_integrity(filename):

	global file_itr
	global cur_filename
	global g_options

	if cur_filename != filename:
		cur_filename = filename
		file_itr = tf.python_io.tf_record_iterator(g_options["working_dir"] + filename)

	for record in file_itr:

		m = hashlib.md5()
		m.update(record)

		return "hash=" + m.hexdigest()

	return "result=eof"

def end_writing():
	return True

def finalize():
	print("Script finalized")
	return True
