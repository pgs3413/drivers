KERN_DIR = /usr/src/linux-headers-5.15.0-105-generic

obj-m += $(ko).o
SRC_FILES := $(wildcard $(ko)/*.c)
OBJ_FILES ?= $(patsubst %.c,%.o,$(SRC_FILES))
$(ko)-objs := $(OBJ_FILES)

all:
	$(MAKE) -C $(KERN_DIR) M=$(PWD) OBJ_FILES="$(OBJ_FILES)" modules

clean:
	@rm -f ./.*.cmd
	@rm -f ./*.mod*
	@rm -f ./*.o
	@rm -f ./*.order
	@rm -f ./*.symvers
	@rm -rf ./*/.*.cmd
	@rm -rf ./*/*.o
	@rm -rf ./*/*.o.d