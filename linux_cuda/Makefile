CC=nvcc
CFLAGS=-m64 -arch=sm_37 -I../include,../include/jpeg,../include/ws -Xcompiler -fpic -c -dc -g -lineinfo# -Xptxas -v
all: jpeg
#	rm -f obj/*.o
	rm -f main

  $(CC) $(CFLAGS) jpeg/jpeg.cu -o obj/jpeg.o
  $(CC) $(CFLAGS) jpeg/jerror.cu -o obj/jerror.o
  $(CC) $(CFLAGS) jpeg/jcomapi.cu -o obj/jcomapi.o
  $(CC) $(CFLAGS) jpeg/jcapimin.cu -o obj/jcapimin.o
  $(CC) $(CFLAGS) jpeg/jcparam.cu -o obj/jcparam.o
  $(CC) $(CFLAGS) jpeg/jmemmgr.cu -o obj/jmemmgr.o
  $(CC) $(CFLAGS) jpeg/jmemansi.cu -o obj/jmemansi.o
  $(CC) $(CFLAGS) jpeg/jcmarker.cu -o obj/jcmarker.o
  $(CC) $(CFLAGS) jpeg/jutils.cu -o obj/jutils.o
  $(CC) $(CFLAGS) jpeg/jdatadst.cu -o obj/jdatadst.o
  $(CC) $(CFLAGS) jpeg/jcapistd.cu -o obj/jcapistd.o
  $(CC) $(CFLAGS) jpeg/jcinit.cu -o obj/jcinit.o
  $(CC) $(CFLAGS) jpeg/jcmaster.cu -o obj/jcmaster.o
  $(CC) $(CFLAGS) jpeg/jccolor.cu -o obj/jccolor.o
  $(CC) $(CFLAGS) jpeg/jcsample.cu -o obj/jcsample.o
  $(CC) $(CFLAGS) jpeg/jcprepct.cu -o obj/jcprepct.o
  $(CC) $(CFLAGS) jpeg/jcdctmgr.cu -o obj/jcdctmgr.o
  $(CC) $(CFLAGS) jpeg/jfdctint.cu -o obj/jfdctint.o
  $(CC) $(CFLAGS) jpeg/jfdctfst.cu -o obj/jfdctfst.o
  $(CC) $(CFLAGS) jpeg/jfdctflt.cu -o obj/jfdctflt.o
  $(CC) $(CFLAGS) jpeg/jcphuff.cu -o obj/jcphuff.o
  $(CC) $(CFLAGS) jpeg/jchuff.cu -o obj/jchuff.o
  $(CC) $(CFLAGS) jpeg/jccoefct.cu -o obj/jccoefct.o
  $(CC) $(CFLAGS) jpeg/jcmainct.cu -o obj/jcmainct.o

  $(CC) $(CFLAGS) ws/base64.cu -o obj/base64.o
  $(CC) $(CFLAGS) ws/Communicate.cu -o obj/Communicate.o
  $(CC) $(CFLAGS) ws/Datastructures.cu -o obj/Datastructures.o
  $(CC) $(CFLAGS) ws/Errors.cu -o obj/Errors.o
  $(CC) $(CFLAGS) ws/Handshake.cu -o obj/Handshake.o
  $(CC) $(CFLAGS) ws/md5.cu -o obj/md5.o
  $(CC) $(CFLAGS) ws/sha1.cu -o obj/sha1.o
  $(CC) $(CFLAGS) ws/utf8.cu -o obj/utf8.o -g
	$(CC) $(CFLAGS) ws/Websocket.cu -o obj/Websocket.o

	$(CC) $(CFLAGS) init.cu -o obj/init.o
	$(CC) $(CFLAGS) text.cu -o obj/text.o
	$(CC) $(CFLAGS) quaternion.cu -o obj/quaternion.o
	$(CC) $(CFLAGS) mouse.cu -o obj/mouse.o
	$(CC) $(CFLAGS) plot3d.cu -o obj/plot3d.o
	$(CC) $(CFLAGS) utils.cu -o obj/utils.o
	$(CC) $(CFLAGS) automaton.cu -o obj/automaton.o
	$(CC) $(CFLAGS) tuple.cu -o obj/tuple.o
	$(CC) $(CFLAGS) brick.cu -o obj/brick.o
	$(CC) $(CFLAGS) vector3d.cu -o obj/vector3d.o
	$(CC) $(CFLAGS) scenarios.cu -o obj/scenarios.o
	$(CC) $(CFLAGS) rotation.cu -o obj/rotation.o
	$(CC) $(CFLAGS) tree.cu -o obj/tree.o
#
#	nvcc -arch=sm_37 -dlink obj/*.o -o obj/gpu.o
	nvcc -arch=sm_37 obj/*o -lm -L/usr/local/cuda/lib64 -lpthread -lcuda -lcudart -o main

#	rm obj/*.o
	@echo 'done'
