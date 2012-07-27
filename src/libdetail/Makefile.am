SUBDIRS=include src/frame src/plugin
CURRENTPATH=$(shell /bin/pwd)
INCLUDES=-I$(CURRENTPATH)/include -I$(CURRENTPATH)/include/libdetail
export INCLUDES

detail_plugindir=$(prefix)/detail_plugin
detail_plugin_DATA=src/plugin/.libs/*.so.0.0.0