EXE_NAME := artik_switch

ARTIK_SDK_LIBS := \
	libartik-sdk-base \
	libartik-sdk-systemio\
	libartik-sdk-connectivity

OBJS := \
	artik_switch.o

CC := ${CROSS_COMPILE}gcc
ifdef SYSROOT
PKG_CONFIG := PKG_CONFIG_PATH=${SYSROOT}/usr/lib/pkgconfig/ PKG_CONFIG_SYSROOT_DIR=${SYSROOT} pkg-config
CFLAGS := ${CFLAGS}# --sysroot=${SYSROOT}
LDFLAGS := ${LDFLAGS} --sysroot=${SYSROOT} -L${SYSROOT}/usr/lib
else
PKG_CONFIG := PKG_CONFIG_PATH=${SYSROOT}/usr/lib/pkgconfig/ pkg-config
endif

CFLAGS := ${CFLAGS} $(shell ${PKG_CONFIG} --cflags --libs ${ARTIK_SDK_LIBS})
LDFLAGS := ${LDFLAGS} $(shell ${PKG_CONFIG} --libs ${ARTIK_SDK_LIBS})

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXE_NAME): $(OBJS)
	${CC} -o $@ $^ $(LDFLAGS)

all: $(EXE_NAME)

clean:
	rm -f *.o $(EXE_NAME)

