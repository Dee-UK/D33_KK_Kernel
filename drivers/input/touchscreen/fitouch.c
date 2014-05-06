#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

#define DRIVER_VERSION		"v1.0"
#define DRIVER_AUTHOR		"Zou JunBiao <yanfa3@fitouch.com.cn>"
#define DRIVER_DESC			"Fitouch Driver"

enum
{
    CONFIG_DEVTYPE_SG_FITOUCH     = 0,
    CONFIG_DEVTYPE_Z_MT_FITOUCH   = 1,
    CONFIG_DEVTYPE_MT_FITOUCH     = 2
};

static int fit_dev_type = CONFIG_DEVTYPE_SG_FITOUCH;

/* device specifc data/functions */
struct fitouch_usb;

struct fitouch_device_info {
	int min_xc, max_xc;
	int min_yc, max_yc;
	int min_press, max_press;
	int rept_size;
	void (*process_pkt) (struct fitouch_usb *usbtouch, unsigned char *pkt, int len);

	/*
	 * used to get the packet len. possible return values:
	 * > 0: packet len
	 * = 0: skip one byte
	 * < 0: -return value more bytes needed
	 */
	int  (*get_pkt_len) (unsigned char *pkt, int len);

	int  (*read_data)   (struct fitouch_usb *usbtouch, unsigned char *pkt);
	int  (*init)        (struct fitouch_usb *usbtouch);
};


/* a usbtouch device */
struct fitouch_usb {
	unsigned char *data;
	dma_addr_t data_dma;
	unsigned char *buffer;
	int buf_len;
	struct urb *irq;
	struct usb_device *udev;
	struct input_dev *input;
	struct fitouch_device_info *type;
	char name[128];
	char phys[64];
 	
	int x[10], y[10];
	int touch[10];
	int press,right;
};

static void usbtouch_process_pkt(struct fitouch_usb *usbtouch,unsigned char *pkt, int len);
static int fitouch_read_data(struct fitouch_usb *dev, unsigned char *pkt);

/* device types */
enum {
	DEVTYPE_IGNORE = -1,
	DEVTYPE_SG_FITOUCH,
	DEVTYPE_Z_MT_FITOUCH,
	DEVTYPE_MT_FITOUCH
};

#define USB_DEVICE_HID_CLASS(vend, prod) \
	.match_flags = USB_DEVICE_ID_MATCH_INT_CLASS \
		| USB_DEVICE_ID_MATCH_INT_PROTOCOL \
		| USB_DEVICE_ID_MATCH_DEVICE, \
	.idVendor = (vend), \
	.idProduct = (prod), \
	.bInterfaceClass = USB_INTERFACE_CLASS_HID, \
	.bInterfaceProtocol = USB_INTERFACE_PROTOCOL_MOUSE

static const struct usb_device_id usbtouch_devices[] = {
	{USB_DEVICE(0x10C4, 0x5E52), .driver_info = DEVTYPE_Z_MT_FITOUCH},
	{USB_DEVICE(0x10C4, 0x5E51), .driver_info = DEVTYPE_MT_FITOUCH},
	{USB_DEVICE(0x10C4, 0x5E50), .driver_info = DEVTYPE_SG_FITOUCH},
	{}
};

/*
*
*
*/
static struct fitouch_device_info usbtouch_dev_info[] = {
	
		[DEVTYPE_MT_FITOUCH] = {
			.min_xc		= 0x0,
			.max_xc		= 0x7FFF,
			.min_yc		= 0x0,
			.max_yc		= 0x7FFF,
			.rept_size	= 14,
			.read_data	=  fitouch_read_data,
			.process_pkt 	= usbtouch_process_pkt,
		},
		[DEVTYPE_SG_FITOUCH] = {
			.min_xc		= 0x0,
			.max_xc		= 0x7FFF,
			.min_yc		= 0x0,
			.max_yc		= 0x7FFF,
			.rept_size	= 6,
			.read_data	= fitouch_read_data,
			.process_pkt 	= usbtouch_process_pkt,
		},
		
		[DEVTYPE_Z_MT_FITOUCH] = {
			.min_xc		= 0x0,
			.max_xc		= 0x7FFF,
			.min_yc		= 0x0,
			.max_yc		= 0x7FFF,
			.rept_size	= 62,
			.read_data	= fitouch_read_data,
			.process_pkt 	= usbtouch_process_pkt,
		},
	
};

/*
*
*
*/
static int fitouch_read_data(struct fitouch_usb *dev, unsigned char *pkt)
{
	//printk("fitouch_read_data 777777777777777777777777777777777");	
	if (fit_dev_type == CONFIG_DEVTYPE_MT_FITOUCH)
	{
			dev->x[0]= ((pkt[4]&0x7F)<<8) |(int)(pkt[3]);
			dev->y[0]= ((pkt[6]&0x7F)<<8) |(int)(pkt[5]);
			dev->touch[0] = pkt[1] & 0x07;
			
		  dev->x[1]= ((pkt[10]&0x7F)<<8) |(int)(pkt[9]);
			dev->y[1]= ((pkt[12]&0x7F)<<8) |(int)(pkt[11]);
			dev->touch[1] = pkt[7] & 0x07;
	}
 if(fit_dev_type == CONFIG_DEVTYPE_SG_FITOUCH)
 {	
		dev->x[0] = (pkt[3] <<8) | pkt[2];
		dev->y[0] = (pkt[5] <<8) | pkt[4];
		dev->touch[0] = pkt[1] & 0x07;
 }
 
	if (fit_dev_type == CONFIG_DEVTYPE_Z_MT_FITOUCH)
	{	
		int i;
		for(i = 0;i< pkt[61];i++)
		{
			dev->x[i]= ((pkt[4+6*i]&0x7F)<<8) |(int)(pkt[3+6*i]);
			dev->y[i]= ((pkt[6+6*i]&0x7F)<<8) |(int)(pkt[5+6*i]);
			dev->touch[i] = pkt[1+6*i] & 0x07;
		}
	}
	return 1;
}

/*****************************************************************************
 * Generic Part
 */
static void usbtouch_process_pkt(struct fitouch_usb *usbtouch,
                                 unsigned char *pkt, int len)
{
	struct fitouch_device_info *type = usbtouch->type;

	if (!type->read_data(usbtouch, pkt))
			return;
	if(pkt[0]==0xf2)
	{//多点包处理
		if(fit_dev_type == CONFIG_DEVTYPE_MT_FITOUCH)
		{	
			//printk("usbtouch_process_pkt 6666666666666666666666666666");	
			if (pkt[13] == 1)
			{
				if(usbtouch->touch[0] == 0)
				{
					input_mt_sync(usbtouch->input);//SYN_MT_REPORT
				}else
				{
					input_report_abs(usbtouch->input, ABS_MT_POSITION_X, usbtouch->x[0]);
					input_report_abs(usbtouch->input, ABS_MT_POSITION_Y, usbtouch->y[0]);
					//SYN_MT_REPORT
					input_report_abs(usbtouch->input, ABS_MT_PRESSURE,usbtouch->touch[0]);
				}
			}
			if (pkt[13] == 2)
			{
				if(usbtouch->touch[0] == 0)
				{
					input_mt_sync(usbtouch->input);
				}else
				{
					input_report_abs(usbtouch->input, ABS_MT_POSITION_X, usbtouch->x[0]);
					input_report_abs(usbtouch->input, ABS_MT_POSITION_Y, usbtouch->y[0]);
					input_report_abs(usbtouch->input, ABS_MT_PRESSURE,usbtouch->touch[0]);
				}
				if(usbtouch->touch[1] == 0)
				{
					input_mt_sync(usbtouch->input);
				}else
				{
					input_report_abs(usbtouch->input, ABS_MT_POSITION_X, usbtouch->x[1]);
					input_report_abs(usbtouch->input, ABS_MT_POSITION_Y, usbtouch->y[1]);
					input_report_abs(usbtouch->input, ABS_MT_PRESSURE,usbtouch->touch[1]);
					input_set_abs_params(usbtouch->input, ABS_MT_WIDTH_MAJOR,0, 15, 0, 0);
					input_mt_sync(usbtouch->input);
				}
			}
		}
		if (fit_dev_type == CONFIG_DEVTYPE_Z_MT_FITOUCH)
		{	
			//printk("usbtouch_process_pkt 5555555555555555555555555555");	
			unsigned char i;
			if(pkt[0]==0xf2)
			{
				if(pkt[61] == 0)
				{
					input_mt_sync(usbtouch->input);
				}else
				{
					for(i=0;i<pkt[61];i++)
					{
						if(usbtouch->touch[i] == 0)
						{
							input_mt_sync(usbtouch->input);
						}else
						{
							input_report_abs(usbtouch->input, ABS_MT_POSITION_X, usbtouch->x[i]);
							input_report_abs(usbtouch->input, ABS_MT_POSITION_Y, usbtouch->y[i]);
							//input_report_abs(usbtouch->input, ABS_MT_PRESSURE, usbtouch->touch[i]);ABS_MT_TRACKING
							//input_report_abs(usbtouch->input, ABS_MT_TRACKING, i);
							input_report_abs(usbtouch->input, ABS_MT_PRESSURE, usbtouch->touch[i]);
							if(i>=1)
								input_set_abs_params(usbtouch->input, ABS_MT_WIDTH_MAJOR,0, 15, 0, 0);
							input_mt_sync(usbtouch->input);
						}
					}
				}
			}
		}
	}
	else if(pkt[0]==0xf4){
		/*if(usbtouch->touch[0] != 0)
		{
			//printk("usbtouch_process_pkt 444444444444444444444444");	
			input_report_key(usbtouch->input, BTN_TOUCH,usbtouch->touch[0]);
			input_report_abs(usbtouch->input, ABS_X, usbtouch->x[0]);
			input_report_abs(usbtouch->input, ABS_Y, usbtouch->y[0]);
			if (type->max_press)
				input_report_abs(usbtouch->input, ABS_PRESSURE, usbtouch->press);
		}*/
		//input_report_abs(usbtouch->input, BTN_TOUCH,0);
		//else 
		input_report_key(usbtouch->input, BTN_TOUCH,usbtouch->touch[0]);
		input_report_abs(usbtouch->input, ABS_X, usbtouch->x[0]);
		input_report_abs(usbtouch->input, ABS_Y, usbtouch->y[0]);
		if (type->max_press)
			input_report_abs(usbtouch->input, ABS_PRESSURE, usbtouch->press);
	}
	input_sync(usbtouch->input);
}

static void usbtouch_irq(struct urb *urb)
{
	struct fitouch_usb *usbtouch = urb->context;
	int retval;
	
	switch (urb->status) {
	case 0:
		/* success */
		break;
	case -ETIME:
		/* this urb is timing out */
		dbg("%s - urb timed out - was the device unplugged?",
		    __FUNCTION__);
		return;
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		/* this urb is terminated, clean up */
		dbg("%s - urb shutting down with status: %d",
		    __FUNCTION__, urb->status);
		return;
	default:
		dbg("%s - nonzero urb status received: %d",
		    __FUNCTION__, urb->status);
		goto exit;
	}

	usbtouch->type->process_pkt(usbtouch, usbtouch->data, urb->actual_length);

exit:
	retval = usb_submit_urb(urb, GFP_ATOMIC);
	if (retval)
		err("%s - usb_submit_urb failed with result: %d",
		    __FUNCTION__, retval);
}

static int usbtouch_open(struct input_dev *input)
{
	struct fitouch_usb *usbtouch = input_get_drvdata(input);

	usbtouch->irq->dev = usbtouch->udev;

	if (usb_submit_urb(usbtouch->irq, GFP_KERNEL))
		return -EIO;

	return 0;
}

static void usbtouch_close(struct input_dev *input)
{
	struct fitouch_usb *usbtouch = input_get_drvdata(input);

	usb_kill_urb(usbtouch->irq);
}


static void usbtouch_free_buffers(struct usb_device *udev,
				  struct fitouch_usb *usbtouch)
{
usb_free_coherent(udev, usbtouch->type->rept_size,
			  usbtouch->data, usbtouch->data_dma);
	kfree(usbtouch->buffer);
}
#define D_name "fitouch"
static int usbtouch_probe(struct usb_interface *intf,
			  const struct usb_device_id *id)
{
	struct fitouch_usb *usbtouch;
	struct input_dev *input_dev;
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_device *udev = interface_to_usbdev(intf);
	struct fitouch_device_info *type;
	int err = -ENOMEM;

    //printk("usbtouch_probe 1111111111111111");
	/* some devices are ignored */
	if (id->driver_info == DEVTYPE_IGNORE)
		return -ENODEV;
	interface = intf->cur_altsetting;
	endpoint = &interface->endpoint[0].desc;
	usbtouch = kzalloc(sizeof(struct fitouch_usb), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!usbtouch || !input_dev)
		goto out_free;

	type = &usbtouch_dev_info[id->driver_info];
	usbtouch->type = type;
	usbtouch->data = usb_alloc_coherent(udev, type->rept_size,GFP_KERNEL, &usbtouch->data_dma);
	if (!usbtouch->data)
		goto out_free;

  printk("1 type->get_pkt_len = %d\n",(int)type->get_pkt_len);
	if (type->get_pkt_len) {
		usbtouch->buffer = kmalloc(type->rept_size, GFP_KERNEL);
		if (!usbtouch->buffer)
			goto out_free_buffers;
	}
	usbtouch->irq = usb_alloc_urb(0, GFP_KERNEL);
	if (!usbtouch->irq) {
		dbg("%s - usb_alloc_urb failed: usbtouch->irq", __func__);
		goto out_free_buffers;
	}

	usbtouch->udev = udev;
	usbtouch->input = input_dev;

	if (udev->manufacturer)
		strlcpy(usbtouch->name, udev->manufacturer, sizeof(usbtouch->name));
	if (udev->product) {
		if (udev->manufacturer)
			strlcat(usbtouch->name, " ", sizeof(usbtouch->name));
			strlcat(usbtouch->name, udev->product, sizeof(usbtouch->name));
	}

	if (!strlen(usbtouch->name))
		snprintf(usbtouch->name, sizeof(usbtouch->name),
				"fitouch screenn %04x:%04x",
			 le16_to_cpu(udev->descriptor.idVendor),
			 le16_to_cpu(udev->descriptor.idProduct));
	usb_make_path(udev, usbtouch->phys, sizeof(usbtouch->phys));
	strlcat(usbtouch->phys, "/input0", sizeof(usbtouch->phys));

	input_dev->name = usbtouch->name;
	input_dev->name = D_name;
	
	input_dev->phys = usbtouch->phys;
	usb_to_input_id(udev, &input_dev->id);
	input_dev->dev.parent = &intf->dev;
	input_set_drvdata(input_dev, usbtouch);
	input_dev->open = usbtouch_open;
	input_dev->close = usbtouch_close;

  if(udev->descriptor.idProduct == 0x5e50)
  {
  	   fit_dev_type = CONFIG_DEVTYPE_SG_FITOUCH;
  	   //printk("usbtouch_probe 0x5e50 1111111111111111");
  }
  else if(udev->descriptor.idProduct == 0x5e51)
  {
  	  fit_dev_type = CONFIG_DEVTYPE_MT_FITOUCH;  
  	  //printk("usbtouch_probe 0x5e51 1111111111111111");	
  }
  else if(udev->descriptor.idProduct == 0x5e52)
  {
  		fit_dev_type = CONFIG_DEVTYPE_Z_MT_FITOUCH;
  		//printk("usbtouch_probe 0x5e52 1111111111111111");	
  }

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

if(fit_dev_type == CONFIG_DEVTYPE_SG_FITOUCH)
{	
		//printk("usbtouch_probe 333333333333333333333333333");	
		input_set_abs_params(input_dev, ABS_X, type->min_xc, type->max_xc, 0, 0);
		input_set_abs_params(input_dev, ABS_Y, type->min_yc, type->max_yc, 0, 0);
		if (type->max_press)
			input_set_abs_params(input_dev, ABS_PRESSURE, type->min_press,
			                     type->max_press, 0, 0);	                     
}

if ((fit_dev_type == CONFIG_DEVTYPE_Z_MT_FITOUCH)||(fit_dev_type == CONFIG_DEVTYPE_MT_FITOUCH))
{	
		//printk("usbtouch_probe duo 2222222222222222222222");	
		input_set_abs_params(input_dev, ABS_X, type->min_xc, type->max_xc, 0, 0);
		input_set_abs_params(input_dev, ABS_Y, type->min_yc, type->max_yc, 0, 0);
		input_set_abs_params(input_dev, ABS_MT_POSITION_X, type->min_xc, type->max_xc, 0, 0);/* Center X ellipse position */
		input_set_abs_params(input_dev, ABS_MT_POSITION_Y, type->min_yc, type->max_yc, 0, 0);/* Center Y ellipse position */
		input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
		//input_set_abs_params(input_dev, ABS_MT_TRACKING, 0, 10, 0, 0);
		input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR,0, 255, 0, 0);
}

	usb_fill_int_urb(usbtouch->irq, usbtouch->udev,
			 usb_rcvintpipe(usbtouch->udev, endpoint->bEndpointAddress),
			 usbtouch->data, type->rept_size,
			 usbtouch_irq, usbtouch, endpoint->bInterval);
	usbtouch->irq->dev = usbtouch->udev;
	usbtouch->irq->transfer_dma = usbtouch->data_dma;
	usbtouch->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	/* device specific init */	
	err = input_register_device(usbtouch->input);
	if (err) {
		dbg("%s - input_register_device failed, err: %d", __func__, err);
		goto out_free_buffers;
	}
	usb_set_intfdata(intf, usbtouch);
	return 0;

out_free_buffers:
	usbtouch_free_buffers(udev, usbtouch);
out_free:
	input_free_device(input_dev);
	kfree(usbtouch);
	return err;
}

static void usbtouch_disconnect(struct usb_interface *intf)
{
	struct fitouch_usb *usbtouch = usb_get_intfdata(intf);

	dbg("%s - called", __FUNCTION__);

	if (!usbtouch)
		return;

	dbg("%s - usbtouch is initialized, cleaning up", __FUNCTION__);
	usb_set_intfdata(intf, NULL);
	usb_kill_urb(usbtouch->irq);
	input_unregister_device(usbtouch->input);
	usb_free_urb(usbtouch->irq);
	usbtouch_free_buffers(interface_to_usbdev(intf), usbtouch);
	kfree(usbtouch);
}

MODULE_DEVICE_TABLE(usb, usbtouch_devices);

static struct usb_driver usbtouch_driver = {
	.name		= "fitouch",
	.probe		= usbtouch_probe,
	.disconnect	= usbtouch_disconnect,
	.id_table		= usbtouch_devices,
};

static int __init usbtouch_init(void)
{
	return usb_register(&usbtouch_driver);
}

static void __exit usbtouch_cleanup(void)
{
	usb_deregister(&usbtouch_driver);
}

module_init(usbtouch_init);
module_exit(usbtouch_cleanup);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

MODULE_ALIAS("touchkitusb");
MODULE_ALIAS("itmtouch");
MODULE_ALIAS("mtouchusb");

