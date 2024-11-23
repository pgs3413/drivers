# linux内核开发相关笔记

## 一、模块

#### 1.模块是什么

可在运行时添加到内核中的代码被称为“模块”。每个模块由目标代码和数据组成，使用insmod程序连接到正在运行的内核，也可以使用rmmod程序移除连接。

模块机制本质就是将一堆.c文件编译打包成一个内核可识别的.ko文件，一个.ko文件是一个ELF relocateble 文件（不是一个ELF可执行文件，因为其中的代码还需要重定位），其中包含代码段、数据段、符号表、重定位表以及一些内核需要的段（比如初始化段）。

在运行时将.ko文件动态装载进内核，内核需要分配内核空间存放代码数据等信息，然后对代码中用到的外部符号（其他模块的符号以及内核静态编译的导出符号）进行符号解析以及重定位（即动态链接），然后调用模块的初始化函数，如此模块代码便能正常工作了。

模块的代码使用机制一般有两种：1.导出符号供其他模块解析调用；2.使用特定的注册机制向内核注册函数（使用函数指针），在进程进行某一些系统调用或者发生中断时，内核会主动调用模块函数，这样模块代码就可以参与到某一个内核路径中了。

#### 2. 构造模块

需要一个内核目录树，包含linux头文件以及makefile文件。因为我们的makefile本质是间接利用目录树中复杂的makefile完成复杂的构造模块过程。

```makefile
obj-m := module.o
module-objs := file1.o file2.o
make -C $(目录树路径) M=`pwd` modules
```

-C 参数来到目录树下make，执行目录树下的根Makefile的modules目标，将当前目录传递给根Makefile，该Makefile就会来到当前目录寻找makefile的obj-m变量，执行后续构造过程

#### 3.装载与卸载

insmod程序与ld有点类似，它将模块的代码与数据装入内核，然后使用内核的符号表解析模块中任何未解析的符号。然而，与链接器不同，内核不会修改模块文件，而且仅仅修改内存中的副本。

modprobe程序则会考虑模块中使用到的非内核导出符号，找到并装载依赖模块。

rmmod移除模块。

lsmod列出所有模块。

#### 4.导出符号

EXPORT_SYMBOL(name);

EXPORT_SYSBOL_GPL(name);

## 二、虚拟文件系统 VFS

#### 1.VFS

VFS是一个内核软件层，向上为用户空间应用层提供标准的VFS系统调用（open、read、write、close），向下为各个具体的文件系统提供一个通用的借口框架。当进程发起系统调用后，VFS负责将标准的系统调用转换为具体文件系统的具体操作。

#### 2.通用文件模型

VFS的主要思想是引入一个通用的文件模型，这个模型能够表示所有支持的文件系统。每个具体的文件系统，必须将其物理组织结构转换为VFS的通用文件模型。

通用文件模型是面向对象的。有统一的对象模型，但每一个对象的数据与操作（函数指针数组）是由具体文件系统决定的。即每一个文件系统都会实例化自己的一套对象。

对象模型组件有：superblock、inode、file、dentry

superblock：一个具体的文件系统对应一个superblock object，用于存放一个已安装文件系统的总体信息。由文件系统实例化。

inode：一个具体的文件对应一个inode object，存放该文件的一般信息。由文件系统对应的superblock的方法实例化。其中的i_fop字段存放file对象需要的操作，通过改变该字段可改变file的行为。

file：该对象非文件系统实例化，而由VFS实例化。存放一些VFS与进程进行交互的有关信息。

dentry：存放与目录树相关的信息。每一个文件系统具有自己的物理组织结构，这些结构都会在逻辑上形成一个目录树（与进程所在namespace的目录树不是一个层级），目录树上的每一个节点（硬链接）都对应一个dentry object，由文件系统实例化。所以通过一个文件系统的所有dentry object就可以映射出该文件系统的目录树。众多文件系统的dentry目录树通过层叠的方式最终形成一个进程所能看到的目录树，该目录树是虚拟的，其中的节点不会对应一个dentry对象，而是由各个文件系统的dentry逻辑上共同构成。

file的f_dentry字段指向对应的dentry对象，dentry的d_inode字段指向对应的inode对象。

#### 3.路径名查找 path_lookup

很多的系统调用都是通过一个文件路径名用于识别一个具体文件，所以path_lookup需要根据路径名字符串找到该文件相应的identry（相当于找到了对应的inode）及该dentry所属的文件系统对应的vfsmount。

查找过程：

1.   找到dentry起点，如果是绝对路径，起点为current->fs->root；如果是相对路径，起点为current->fs->pwd。

2. 通过dentry找到下一个dentry（通过dentry->d_inode中的方法），如此反复直到找到最后一个dentry。解析过程（对进程所看到的目录树的搜索过程）中如果遇到安装点就会进行文件系统的切换，如果碰到符号文件就会进行软链接的解析。

#### 4.open\close

open系统调用的服务例程是sys_open，参数要打开文件的路径名、访问标识、新文件的许可权掩码。

sys_open打开过程：

1. 调用filp_open：使用path_lookup根据路径名找到对应的dentry及inode（dentry->d_inode）

2. 调用dentry_open：分配一个新的file对象，设置file->f_op字段为inode->i_fop

3. 如果file->f_op->open有定义，调用它

4. current->files->fd[fd]置位file对象的地址，返回fd

close就是简单的调用file->f_op->close然后回收file对象。

#### 5.read\write

对应的服务例程为sys_read/sys_write

read/write 过程:

1. 根据fd找到对应的file

2. 调用file->f_op->read/file->f_op->write

## 三、设备与驱动程序

#### 1.设备

设备可以是一个实际的物理设备，也可以是一个软件模拟的虚拟设备。与CPU连接完成IO操作。

设备类型：

    1.块设备：该设备可抽象为一个具有固定大小字节的块的数组。每一个块具有逻辑块号  （数组下标），所以该设备的数据可以被随机访问。

    2.字符设备：该设备可抽象为一个字节流。一般按字节进行顺序访问。

    3.网络设备：完成网络IO的设备。

#### 2.驱动程序

驱动字符设备运行的内核代码。

#### 3.设备号

每一个设备（物理设备还是虚拟设备）都需要分配一个设备号，设备号为该设备的标识符。一个设备号（32位的dev_t）由12位主设备号和20位的次设备号组成。

块设备与字符设备分别具有一个独立的设备号空间。

每个设备驱动程序在注册阶段都会指定它将要处理的设备号范围。

#### 4.设备文件

一个设备可抽象成一个设备文件，通过使用设备文件（与使用普通文件一样的方式）使用驱动程序以驱动设备。

设备文件是存放在文件系统的一个实际文件，与普通文件一样，只是文件内容与类型不一样。设备文件的内容是一个设备号（可以直接存放在对应索引节点，而不需要分配数据块），类型则是块设备文件（b）或者字符设备文件（c）。路径为/dev/xxx。

通过访问一个设备文件（站在文件系统的角度看，与访问一个普通文件一般），内核可以得到设备类型与设备号，然后通过一些特殊处理可以将该设备文件关联到对应的驱动程序。

mknod系统调用用来创建设备文件，参数有设备文件名，设备类型，主设备号及次设备号。

#### 5.设备文件的VFS处理

站在VFS的角度，虽然设备文件也在系统的目录树中，但是它们和普通文件有根本的不同。在进程访问设备文件时，需要把操作转给对应的驱动程序，而不是设备文件所在的文件系统。

为了做到这一点，需要在打开一个设备文件时，改变file的文件操作（f_op）为一些特殊的操作。但是VFS把这个特殊处理的过程对上层屏蔽了。

在open一个设备文件，调用path_lookup根据设备文件名已经获得了对应的inode（与一般文件一样的查找过程），当发现该inode是设备类型，则把inode->i_rdev初始化为设备号，把inode->i_fop初始化为def_blk_fops（如果是块设备）或者def_chr_fops（如果是字符设备）。

正是这两个表的引入，才使得在设备文件上所发出的任何系统调用都将激活设备驱动程序的函数而不是基本文件系统的函数。

## 四、字符设备驱动程序

字符设备驱动程序是用于驱动字符设备的驱动程序。在进程能正确使用驱动程序前，需要进行一些前置活动。

#### 1.请求分配设备号

 注册设备号以用于应用层识别设备。需要向内核请求分配一个设备号范围，由内核保证设备号的独立分配，驱动程序再自行将设备号与设备映射上，并告知应用程序映射规则。

**alloc_chrdev_region**函数可以动态分配一个主设备号，参数为初始次设备号、范围的大小以及设备驱动程序的名称。

#### 2.生成设备文件

这一步一般是在用户态进行。生成一个指定路径名、指定设备号的字符设备文件。

#### 2.提供一组函数

字符设备驱动程序需要提供一组用于操作字节流的函数（open、read、write、close等），并用**file_operations**结构体组织起来，本质是一组函数指针。

#### 3.注册cdev

驱动程序需要把**cdev**注册进内核，这样内核才能通过设备号找到该驱动程序所提供的操作。

**cdev**结构描述了一组file操作和一组设备号的关联，其中的file_operations * ops指向该驱动程序相关操作的file_operations结构；dev字段指向该驱动程序所分配的主设备号和次设备号；count字段为设备号范围大小。

**cdev_alloc**函数的功能是动态地分配并初始化cdev描述符。

**cdev_add**函数的功能是在设备驱动模型中注册一个cdev描述符。它初始化cdev描述符中的dev和count字段，然后调用kobj_map函数。kobj_map负责建立相关的数据结构，把cdev加入到cdev_maps散列表中。该散列表key为设备号，value为cdev对象

#### 4.访问驱动程序

驱动程序注册后，便可以通过设备号（通过访问设备文件）访问驱动程序。

当每次打开一个字符设备文件时，都会调用到def_chr_fops->open函数，实际上def_chr_fops表只定义了一个open函数--chardev_open：

使用inode->i_rdev(设备号)在cdev_maps中找到对应的cdev结构，然后将inode->i_cdev设置为该cdev的地址, file->f_op初始化为cdev->ops。如此之后，该file对象中操作表中的函数就是驱动程序所提供的函数。其后的open、read、write、close操作就被转发到驱动程序所提供的操作了。

## 五、页高速缓存

#### 1.是什么

页高速缓存就是使用一组页框（内存RAM）来缓存磁盘数据。对磁盘数据的读写都先将经过页高速缓存。

一个文件（普通文件、目录文件、设备文件、特殊文件）都会有一个对应的inode对象，每一个inode对象都会有一个专属的页高速缓存。

每个页具有逻辑页号(指的是该页在对应页高速缓存中的位置)，根据逻辑页号进行排序后组织成一个radix tree。

当需要访问指定逻辑页号的页，但该页不在高速缓存中，新页就被加入到高速缓存中，然后从磁盘中读出数据填充它。

根据inode的不同，页高速缓存也就有了不同的类型。

#### 2.类型

1. 普通文件inode：对应页高速缓存中的页存放的是文件内容数据。把文件内容划分成一个个页，逻辑页号与文件页号一一对应。

2. 块设备文件inode：对应的页高速缓存中的页存放是块设备原始数据。把块设备(可以是整个磁盘或者磁盘分区)划分为一个一个页，每个页有多个块（在块设备看来，这些页是物理连续的），逻辑页号与设备页号一一对应。

#### 3.address_space对象

页高速缓存的核心数据结构是address_space对象，它是一个嵌入到inode中（idata字段）的数据结构, inode->i_mapping为该inode要用到的address_space（通常为自己的idata）。

page_tree字段把属于该高速缓存的页（使用struct page）组织成一颗radix tree，使得通过逻辑页号能够快速找到对应的页以及判断页是否在高速缓存中。还可以快速获得指定状态的页。

a_ops字段指向一个address_space_operations结构，包含跟页高速缓存相关的众多函数，比如当页不存在时调用的readpage函数，把脏页回写到磁盘的writepage函数。在大多数情况下，这些方法把inode及对应的页高速缓存和访问物理设备的低级驱动程序联系起来。

#### 4.处理函数

1. find_get_page根据页索引在指定address_space中查找页，若找不到返回NULL。

2. add_to_page_cache把一个新页加入到address_space中。

3. remove_from_page_cache从address_space中删除一页。

4. read_cache_page确保address_space中包含最新版本的指定页。

#### 5.buffer_head

因为对块设备的访问是以块为基本单元，所以将块设备文件的页高速缓存中的页视为块缓冲区页，即将一页划分为多个块缓冲区（与块同等大小的内存字节数组），一个块缓冲区对应块设备中的一块。

每个块缓冲区都由一个buffer_head对象描述，b_bdev指向对应的block_device，而b_blocknr存放逻辑块号，b_page指向对应的页，b_data表示块缓冲区在页中的位置。

一个页相关的所有buffer_head收集成一个单向循环链表，page->private指着第一个元素。

## 六、块设备驱动程序

块设备驱动程序向上层提供了一个块数组的抽象，块为基本访问单元。整个磁盘是一个块设备，其中每一个分区也是一个块数组。向下使用扇区对设备进行实际访问。通用块层负责将块号转为扇区号。

#### 1.访问方式

有两种方式使用块设备驱动程序，进而使用相应的块设备：

1. 通过访问块设备文件，这样将不通过文件系统直接使用到驱动程序。当打开一个块设备文件时，VFS会将相应的inode->i_fop初始化为def_blk_fops，所以相应fille->f_op为def_blk_fops。访问路径：对设备文件的系统调用->VFS->页高速缓存->通用块层->I/O调度程序层->块设备驱动程序->块设备。

2. 将该块设备组织成一个文件系统，然后将该文件系统进行挂载后，通过访问文件系统中的文件或目录进而使用到驱动程序和块设备。访问路径：对普通文件的系统调用->VFS->文件系统层->页高速缓存->通用块层->I/O调度程序层->块设备驱动程序->块设备。

#### 2.数据结构

1. <mark>gendisk</mark>：gendisk对象描述了整个磁盘，major为主设备号，first_minor为磁盘次设备号，minors为次设备号范围（其他次设备号代表分区），part指向hd_struct指针数组，fops指向block_device_operations对象，queue指向request_queue对象。

2. <mark>block_device_operations</mark>：磁盘操作表，只定义有open、release、ioctl函数指针。

3. <mark>hd_struct</mark>：如果一个磁盘分为了多个分区，每一个分区由一个hd_struct描述，主要描述分区起点、分区长度、分区索引。

4. <mark>request_queue</mark>：双向循环请求队列链表，聚合了相应gendisk的IO请求（因为磁盘IO操作是针对整个磁盘而发起的），每一个元素为request对象。elevator指向elevator_t对象，描述了该队列的IO调度算法，即request的排序规则。request_fn为一个函数指针（request_fn_proc *），该函数是真正执行IO请求的方法。

5. <mark>request</mark>：描述了一个在该gendisk的待处理请求，每个request包含一个或多个bio对象，这些bio对象通常是对物理相邻的扇区发起的IO请求。起初一个request只有一个bio对象，随后将能够合并操作的其他bio对象加入到该请求中，从而扩展该请求。rq_for_each_bio宏可遍历所有bio对象。一个request是可以分多次执行的，所以字段值是动态变化的。

6. <mark>bio</mark>：通用块层核心数据结构，描述了一个针对磁盘上一组连续的块的IO请求，是向通用块层请求IO的基本单元。bi_sector为IO操作的第一个扇区，bi_size为需要传送的字节数，bi_rw为IO操作标识。bi_io_vec指向一个bio_vec数组，bi_vcnt为数组大小，bi_idx为当前使用的bio_vec的索引。

7. <mark>bio_vec</mark>：bio_vec描述了一个段，即页高速缓存中一页的一块字节数组，即与设备进行数据传输的一块内存。bv_page指向相应页框的page对象，bv_offset为页框内数据的偏移量，bv_len为字节长度。

8. <mark>block_device</mark>：block_device描述了一个块设备，每个磁盘以及每个磁盘分区都会有一个block__device对象。该结构起粘合剂的作用，向上与设备文件inode关联（即与VFS关联），向下与gendisk关联（即与块设备驱动程序关联），同时还关联了bdev特殊文件系统的一个inode节点。
   
   bd_dev为该设备的设备号，bd_inode指向bdev文件系统中该设备对应的inode，bd_contaions指向包含该分区的磁盘的block_device（如果为磁盘block_device，则指向自己）,bd_part指向对应的hd_struct（如果为磁盘block_device，则为NULL），bd_disk指向对应的gendisk对象。

9. <mark>bdev特殊文件系统</mark>：每一个设备号可以对应多个设备文件inode，但只能对应一个block_device，所以引入了bdev特殊文件系统。在该文件系统中，一个设备号只有唯一的inode和唯一的block_device，同时也只有唯一的页高速缓存。

10. <mark>def_blk_fops</mark>：是一个file_operations结构，包含块设备文件VFS系统调用相关的函数。
    
    open: blkdev_open
    
    read: generic_file_read
    
    write: blkdev_file_write
    
    release: blkdev_close

#### 3.重要方法

1. <mark>alloc_disk</mark>：分配并初始化一个gendisk对象，如果该gendisk被分成了几个分区，该函数还会分配并初始化一个适当的hd_struct数组。

2. <mark>add_disk</mark>：将gendisk对象插入到通用块层中，主要是插入到bdev_map中，这样可根据设备号得到对应的gendisk对象。

3. <mark>bd_get</mark>：参数为主设备号和次设备号，在bdev特殊文件系统查寻相关的inode，如果不存在这样的inode，则分配一个新inode和新block_device。返回block_device。

#### 4.注册和初始化块设备驱动程序

###### 4.1.预订主设备号

使用register_blkdev预订一个主设备号，设备号是串联全过程的关键资源。但此时预订的主设备号和驱动程序的数据结构之间还没有建立链接。

###### 4.2.块设备文件

在用户空间使用mknod创建相应的块设备文件。

###### 4.3.分配和初始化gendisk

使用alloc_disk分配gendisk对象，初始化gendisk的major、first_minor、minors、capacity、name、fops等。

要明确块大小，以及整磁盘和分区的大小。

###### 4.4.分配和初始化请求队列

使用blk_init_queue函数分配并初始化一个request_queue，其中许多字段初始化为默认值（例如IO调度算法），request_fn设为传参进去的策略例程。然后再自行设置一些特征值。将gendisk->queue设为该队列。

###### 4.5.设置中断处理程序

使用request_irq设置中断处理程序，以待IO完成时得到通知。

##### 4.6.注册gendisk

使用add_disk注册gendisk。一旦注册成功，设备驱动程序就可以开始工作了。

#### 5.打开流程

当打开一个块设备文件时，file对象的f_op被初始化为了def_blk_fops，然后调用def_blk_fops中的open函数:

<mark>blkdev_open</mark>:

1. 根据inode->i_rdev（设备号）调用bdget函数获得对应的唯一block_device（bdev），inode->i_bdev = bdev以加速后续查找。

2. filp->f_mapping = inode->i_mapping = bdev->bd_inode->i_mapping，如此之后，在该设备号所对应的块设备上的所有操作都将作用于同一个页高速缓存。

3. 调用get_gendisk获得与这个块设备相关的gendisk对象（通过设备号查找，还可以区分是磁盘还是磁盘分区，但是会找到同一个gendisk），bdev->bd_disk = gendisk。再根据bdev是磁盘还是分区、gendisk、hd_struct、请求队列中的属性对bdev进一步建立链接。

4. 调用 disk->fops->open快设备方法。

至此，整条链路建立起来了：file<->inode<->block_device<->gendisk<->request_queue。

对file的后续每个系统调用（read、write）都将触发def_blk_fops中的默认块设备文件操作。

#### 6.策略例程

策略例程是块设备驱动程序的一个函数或一组函数，它与硬件块设备之间相互作用以满足调度队列中所汇集的请求。

在把新的请求插入到空的请求队列中，策略例程才被启动。只要块设备驱动程序被激活，就应该对队列中所有请求都进行处理，直到队列为空。

一般采用如下策略：

1.处理队列中的第一个请求并设置块设备控制器（扇区、内存地址、中断），以便在数据传送完成时可以产生一个中断，然后策略例程就终止。

2.当磁盘控制器产生中断时，中断处理程序重新调用策略例程。策略例程要么为当前请求再启动一次数据传送，要么当请求的所有数据块已经传送完成时，把该请求从调度队列中删除然后开始处理下一个请求。

一个request由几个bio组成，一个bio又是由几个段组成。所以有两种方式使用DMA：为每个bio中的每个段服务；使用分散-聚集DMA传送方式，为请求的所有bio中的所有段服务。

## 七、块设备文件读写流程

对块设备文件的系统调用->VFS->页高速缓存->通用块层->I/O调度程序层->块设备驱动程序->块设备。

当对块设备文件进行读写系统调用时，相应的服务例程最终会调用file上的read\write方法，即generic_file_read、blkdev_file_write。

#### 1.读

1. <mark>generic_file_read</mark>：
   
   初始化一个kiocb对象用于跟踪IO操作的完成，一个iovec对象描述用户空间缓冲区，ppos描述读取位置。
   
   调用do_generic_file_read函数从磁盘读入所请求页并把它们拷贝到用户态缓冲区。

2. <mark>do_generic_file_read</mark>：
   
   获得块设备文件的address_space对象，即filp->f_mapping。
   
   根据ppos偏移量和长度计算出第一个请求页的逻辑页号以及页数量，开启一个循环来读入包含请求字节的所有页：
   
     调用find_get_page根据页索引查找指定页：
   
      若所请求的页不在页高速缓存中，则分配一个新页并调用add_to_page_cache将该页插入到页高速缓存中，并调用address_space->readpage函数从磁盘中读出相应页。
   
      若存在，检查所存数据是否是最新的。若页中的数据是无效的，也需要调用address_space->readpage函数从磁盘中读出相应页。
   
     调用file_read_actor函数将该页内容（可能是部分内容）复制到用户缓冲区。

3. <mark>read_page</mark>:
   
   内核会反复使用read_page方法把一个个页从磁盘读到内存中，不同的inode有不同的read_page方法，块设备文件inode（即bdev文件系统的inode）的read_page方法为blkdev_readpage。
   
   该方法是block_read_full_page的封装，参数为页高速缓存中相应的页以及blkdev_get_block函数指针。

4. <mark>block_read_full_page</mark>：
   
   该函数以一次读一块的方式读一页数据。
   
   调用create_empty_buffers将该页划分成多个块缓冲区并分配buffer_head。根据页的逻辑页号（page->index）可计算出每一个buffer_head的b_blocknr（逻辑块号）。
   
   对每一个buffer_head执行submit_bh，提交一次块IO操作。其中bh->b_end_io设为end_buffer_async_read。

#### 2.写

主要流程：把数据从用户态地址空间复制到页高速缓存中的某些页，然后把这些页中的缓冲区标记为脏。

#### 3.对页高速缓存执行块操作

1. <mark>grow_buffers</mark>：把块缓冲区页加到页高速缓存中，参数为block_device，逻辑块号、块大小。先计算逻辑页号index，调用find_or_create_page创建新的页以及新的buffer_head，并初始化相关字段。

2. <mark>__find_get_block</mark>：参数有block_device、逻辑块号、块大小。返回页高速缓存中的块缓冲区对应的buffer_head，如果不存在则返回NULL。

3. <mark>__getblk</mark>：参数有block_device、逻辑块号、块大小。返回页高速缓存中的块缓冲区对应的buffer_head，如果不存在则创建新页和新buffer_head。但是返回的块缓冲区中不必有有效数据。

4. <mark>__bread</mark>：参数有block_device、逻辑块号、块大小。返回页高速缓存中的块缓冲区对应的buffer_head，如果不存在则创建新页和新buffer_head，并从磁盘中读相应块。
   
   调用__getblk获得对应的buffer_head，如果包含有效数据（BH_Uptodate被置位），则返回。
   
   否则将b_end_io置为<mark>end_buffer_read_sync</mark>函数，然后调用submit_bh函数把buffer_head传送到通用块层发起一次块IO请求，然后调用wait_on_buffer把当前进程插入到等待队列，直到IO操作完成，即等到buffer_head的BH_Lock标识被清零。

#### 4.通用块层

1. <mark>submit_bh</mark>：
   
   参数为buffer_head，其中的b_bdev，b_blocknr，b_size已正确初始化。
   
   第一步通过bio_alloc函数分配一个新的bio描述符，然后根据buffer_head初始化bio：
   
   ```c
   bi_sector = bn->b_blocknr * bh->b_size / 512
   bi_bdev = bh->b_bdev
   bi_size = bh->b_size
   bi_io_vec[0].bv_page = bh->b_page
   bi_io_vec[0].bv_len = bh->b_size
   bi_io_vec[0].bv_offset = bh->b_data
   bi_vcnt = 1
   bi_idx = 0
   bi_end_io = end_bio_bh_io_sync
   bi_rw = 数据传输的方向
   ```
   
   第二步调用generic_make_request函数：找到相应gendisk的请求队列queue，调用queue->make_request_fn向IO调度程序发送一个请求，即将该bio加入到请求队列中。
   
   当针对该bio的IO数据传输终止时，内核执行bio->bi_end_io方法，即end_bio_bh_io_sync方法，后者调用bh->b_end_io。

2. <mark>make_request_fn/ _ _make_request</mark>：检查bio对象是否可以并入到已存在的request对象中：如果可以，将bio插入到request的头部或尾部中，可能会进一步合并request；如果不可以则需要分配新的request对象，并将bio插入到该request中。最后块设备驱动程序的策略例程（request_fn）会处理调度队列中的request。

## 八、文件系统

#### 1.概要

文件系统处于VFS和页高速缓存/块设备之间：向下将块设备组织成特定格式，比如规定哪些块用于存放文件元数据，哪些块用于存放文件内容；向上解释特定格式的块设备，将块设备信息抽象成VFS通用文件对象模型，即super_block、inode、dentry等，当用户进程发起VFS系统调用时，VFS将控制权转移给文件模型中文件系统自定义的函数，从而实现各种文件系统功能。

每一个文件系统在逻辑上都可以被看做为一个独立的目录树。将该目录树进行安装之后，便可以在这颗目录树上进行检索：创建、删除、查看、更改文件/目录。

#### 2.挂载/安装

想要访问一个文件系统前，需要将文件系统安装到某个安装点上。

<mark>mount</mark>：参数：设备源路径（代表文件系统之下的块设备，可为null），目标挂载点路径，文件系统类型，挂载标志，私有数据。

1.  调用do_kern_mount创建并返回一个vfsmount安装对象；

2. 调用do_add_mount把vfsmount对象加入到VFS的多个数据结构中便完成了挂载。 

<mark>vfsmount</mark>：对于每个安装操作，内核使用vfsmount对象保存了安装点和安装标志，以及要安装的文件系统与其他已安装文件系统之间的关系。

mnt_mountpoint指向这个文件系统安装点目录的dentry；

mnt_root指向这个文件系统根目录的dentry；

mnt_sb指向这个文件系统的superblock对象；

mnt_namespace指向该vfsmount所属的namespace。

 调用do_kern_mount时：

1. 调用get_fs_type，根据文件系统类型名找到对应的file_system_type对象type；

2. 调用alloc_vfsmount分配一个新的vfsmount对象mnt；

3. mnt->mnt_sb = type->get_sb；

4. 初始化mnt->mnt_root；

5. mnt_namespace = current->namespace

当在open一个普通文件时，会进行vfsmount的切换，并根据该vfsmount的superblock对象和root dentry对象向下搜索，直到找到相应文件的inode，这样便可以完成后续的VFS操作了。

<mark>file_system_type</mark>：用于描述一种文件系统类型。每种文件系统类型可以管理多个块设备，所以可以用多个super_block，但只有一个file_system_type。

name：文件系统类型名

get_sb：根据参数创建初始化并返回一个superblock对象，通过该对象可对相应块设备/文件系统进行操作

kill_sb：删除superblock对象

文件系统模块初始化时需要调用register_filesystem函数来注册自己的file_system_type对象。

#### 3.重要对象

##### superblock

superblock对象描述了一个块设备/文件系统的整体信息。

s_op指向super_operations对象，即定义了该superblock的相关操作

s_root指向了该文件系统的root dentry

##### inode

inode对象描述了文件系统中的一个文件/目录。

i_ino为索引节点号，i_nlink为硬链接数目，i_mode为文件类型与访问权限...

i_op指向inode_operations对象，即定义该inode的相关操作

i_fop指向file_operations对象，即定义与该inode相关的file对象的相关操作

i_data为该inode相关的address_space对象（页高速缓存）

##### dentry

dentry对象用于描述目录树上的每一个节点。即每一个完整文件/目录名（一个硬链接）都有一个dentry对象。

d_inode为与文件名相关联的inode

d_parent为父dentry

d_name为文件名

d_subdirs为子dentry链表

d_op指向一个dentry_operations对象，即定义了该dentry对象的相关操作

#### 4.VFS操作
