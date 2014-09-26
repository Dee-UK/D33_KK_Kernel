/*
 * USB 10M/100M ethernet adapter
 *
 * This file is licensed under the terms of the GNU General Public License 
 * version 2. This program is licensed "as is" without any warranty of any 
 * kind, whther express or implied 
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/stddef.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/usb.h>
#include <linux/crc32.h>
#include <linux/usb/usbnet.h>
#include <linux/slab.h>


#define CH9x00_VID			0x1A86
#define CH9x00_PID_8339		0x8339
#define CH9x00_PID_E091		0xE091
#define CH9x00_PID_E092		0xE092

#define DRIVER_VERSION		"29-May-2013"

#define DEBUG_PRT  //for debug
#undef DEBUG_PRT

#ifdef DEBUG_PRT
#define dbg_prt(format, arg...) printk(KERN_DEBUG format "\n", ## arg)
#else
#define dbg_prt(format, arg...) do {} while (0)
#endif

/**** Reg and CMD definition for CH9x00_PID_E091 and CH9x00_PID_8339****/
#define DEVICE_SPEED_10M 		0x80
#define DEVICE_MEDIA_CONNECTED 	0x40
#define DEVICE_DUPLEX_FULL 		0x20
#define DEVICE_PHY				0x10

#define	REQ_RD_REG		(USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE)
#define	REQ_WR_REG		(USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE)
#define CTRL_TIMEOUT_MS 	1000

#define MCAST_MAX 				0x40
#define	TX_OVERHEAD				0x04
#define	RX_OVERHEAD				0x04

#define GET_MAC_ADDRESS 			0x00
#define SET_HASH_TABLE  			0x04
#define SET_PACKAGE_FILTER 			0x05
	#define RECEIVE_ALL 	0x08
	#define MULTIPKT_EN 	0x04
	#define BROADPKT_EN 	0x02
	#define HASH_MULTI_EN 	0x01
	#define CTRL_CLOSE 		0x00
	#define CTRL_OPEN 		0x01
#define SET_SPEED_DUPLEX 			0x06
	#define HALF_DUPLEX 	0x00
	#define FULL_DUPLEX 	0x01
	#define SPEED_10M 		0x00
	#define SPEED_100M 		0x02
	#define MT 				0x00
	#define AT 				0x04
#define SET_MAC_ADDRESS 			0x07
#define SET_MAC_WAKEUP_FRAME 		0x08
#define SET_WAKEUP_ENABLE 			0x09
	#define LINKCHG0_EN 	0x01
	#define LINKCHG1_EN 	0x02
	#define MAGICPKT_EN 	0x04
	#define WAKEUP1_EN 		0x08
	#define WAKEUP2_EN 		0x10
	#define WAKEUP3_EN 		0x20
	#define WAKEUP4_EN 		0x40
#define SET_FULL_DUPLEX_FLOW_CONTROL 0x0A
#define SET_HALF_DUPLEX_FLOW_CONTROL 0x0B

#define TEST_GET_MAC_ADDRESS
#define TEST_SET_MAC_ADDRESS
//#define TEST_SET_HASH
//#define TEST_SET_PACKAGE_FILTER
#define TEST_SET_SPEED_DUPLEX
//#define TEST_SET_WAKEUP_ENABLE
	#if defined(TEST_SET_WAKEUP_ENABLE)
		#define WKE1_EN
		#define WKE2_EN
		#define WKE3_EN
		#define WKE4_EN
		#define MAGIC_EN
		#define LIKCHG1_EN
		#define LIKCHG0_EN
	#endif


/**** Reg and CMD definition for CH9x00_PID_E092 ****/
// === constant define
#define RX_QLEN(dev) (((dev)->udev->speed == USB_SPEED_HIGH) ? 60 : 4)
#define TX_QLEN(dev) (((dev)->udev->speed == USB_SPEED_HIGH) ? 60 : 4)
	
#define MIN_PACKET sizeof(struct ethhdr)
#define MAX_PACKET 32768

#define TX_TIMEOUT_JIFFIES (5 * HZ)
#define THROTTLE_JIFFIES (HZ / 8)
#define UNLINK_TIMEOUT_MS 3

// for vendor-specific control operations
#define CONTROL_TIMEOUT_MS 1000

// request
#define REQUEST_READ 	0x0E 	
#define REQUEST_WRITE 	0x0F

#define REQUEST_TYPE_READ 		(USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_OTHER)
#define REQUEST_TYPE_WRITE		(USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_OTHER)

// address space
// addr 00---63  : mii addr
// addr 64---128 : mac addr
// note : read/write must be in word (16 bit)

#define MAC_REG_CTRL 64
#define MAC_REG_STATUS 66
#define	MAC_REG_INTERRUPT_MASK 68
#define MAC_REG_PHY_COMMAND 70
#define MAC_REG_PHY_DATA 72
#define MAC_REG_STATION_L 74
#define MAC_REG_STATION_M 76
#define MAC_REG_STATION_H 78
#define MAC_REG_HASH_L 80
#define MAC_REG_HASH_M1 82
#define MAC_REG_HASH_M2 84
#define MAC_REG_HASH_H 86
#define MAC_REG_THRESHOLD 88
#define MAC_REG_FIFO_DEPTH 90
#define MAC_REG_PAUSE 92
#define MAC_REG_FLOW_CONTROL 94

// BIT
// control register bit15 and bit13 reserve
#define LOOPBACK 			(0x01 << 14)
#define BASE100X 			(0x01 << 12)
#define MBPS_10 			(0x01 << 11)
#define DUPLEX_MODE 		(0x01 << 10)
#define PAUSE_FRAME 		(0x01 << 9)
#define PROMISCUOUS 		(0x01 << 8)
#define MULTICAST 			(0x01 << 7)
#define BROADCAST 			(0x01 << 6)
#define HASH 				(0x01 << 5)
#define APPEND_PAD 			(0x01 << 4)
#define APPEND_CRC 			(0x01 << 3)
#define TRANSMITTER_ACTION 	(0x01 << 2)
#define RECEIVER_ACTION 	(0x01 << 1)
#define DMA_ACTION 			(0x01 << 0)

// status register bit15-bit7 reserve
#define ALIGNMENT 			(0x01 << 6) 
#define FIFO_OVER_RUN  		(0x01 << 5) 
#define FIFO_UNDER_RUN 		(0x01 << 4) 
#define RX_ERROR 			(0x01 << 3) 
#define RX_COMPLETE 		(0x01 << 2) 
#define TX_ERROR 			(0x01 << 1) 
#define TX_COMPLETE 		(0x01 << 0)

// fifo depth register bit14 and bit6 reserve
#define ETH_TXBD			(0x01 << 15)
#define ETN_TX_FIFO_DEPTH 	// bit13:8
#define ETH_RXBD 			(0x01 << 7)// bit
#define ETH_RX_FIFO_DEPTH	// bit5:0


// ************************************************** 
int speed_status;
int link_status;
int duplex_status;
int phy_status;


static void ch9x00_async_cmd_callback( struct urb *urb )
{
	struct usb_ctrlrequest *req = (struct usb_ctrlrequest *)urb->context;

	if( urb->status < 0 )
		printk( KERN_DEBUG "%s() failed with %d\n", __FUNCTION__, urb->status);

	kfree(req);
	usb_free_urb(urb);
}


static void ch9x00_set_reg_async(
		struct usbnet *dev, 
		unsigned char request,
		unsigned char requesttype,
		unsigned short value,
		unsigned short index,
		unsigned short size,
		void *data )
{
	struct usb_ctrlrequest *req;
	int ret;
	struct urb *urb;

	urb = usb_alloc_urb( 0, GFP_ATOMIC);
	if(!urb) {
		dev_dbg( &dev->udev->dev, 
				"Error allocation URB in write_cmd_async!\n");
		return;
	}

	req = kmalloc( sizeof *req, GFP_ATOMIC);
	if(!req) {
		dev_err( &dev->udev->dev, 
				"Failed to allocate memory for control request\n");
		goto out;
	}

	req->bRequestType = requesttype;
	req->bRequest = request;
	req->wValue = cpu_to_le16(value);
	req->wIndex = cpu_to_le16(index);
	req->wLength = cpu_to_le16(size);

	usb_fill_control_urb( urb, dev->udev, 
			usb_sndctrlpipe(dev->udev, 0),
			(void *)req, data, size, ch9x00_async_cmd_callback,req);

	ret = usb_submit_urb(urb, GFP_ATOMIC);
	if( ret < 0 ) {
		dev_err( &dev->udev->dev, 
				"Error submitting the control message, ret:%d\n", ret );
		goto out;
	}
	return;

out:
	kfree(req);
	usb_free_urb(urb);
}


static void set_speed_duplex( struct usbnet *dev, unsigned char value_l )
{
	unsigned short value;
	unsigned char value_low  = 0x00;
	unsigned char value_high = 0x00;

	value_low = value_l;
	value = value_low | (value_high << 8 );
	ch9x00_set_reg_async( dev, SET_SPEED_DUPLEX, REQ_WR_REG, value, 0, 0, NULL);

	return;
}


static int control_read( struct usbnet *dev, 
		unsigned char request, unsigned char requesttype, 
		unsigned short value, unsigned short index,
		void *data, unsigned short size, int timeout )
{
	unsigned char *buf = NULL;
	int err = 0;

	dbg_prt("\n--> Control_read() index=0x%02x size=%d\n", index, size );

	buf = kmalloc( size, GFP_KERNEL );
	if( !buf ) {
		err = -ENOMEM;
		goto err_out;
	}

	err = usb_control_msg( dev->udev, 
			usb_rcvctrlpipe(dev->udev, 0),
			request, requesttype, 
			value, index,
			buf, size, timeout);
	if( err == size )
		memcpy( data, buf, size );
	else if( err >= 0 )
		err = -EINVAL;
	kfree(buf);

	return err;

err_out:
	return err;
}


static int control_write( struct usbnet *dev,
		unsigned char request, unsigned char requesttype,
		unsigned short value, unsigned short index,
		void *data, unsigned short size, int timeout )
{
	unsigned char *buf = NULL;
	int err = 0;

	dbg_prt("\n--> Control_write() index=0x%02x size=%d\n", index, size );

	if( data ) {
		buf = kmalloc( size, GFP_KERNEL );
		if( !buf ) {
			err = -ENOMEM;
			goto err_out;
		}
		memcpy( buf, data, size );
	}

	err = usb_control_msg( dev->udev,
			usb_sndctrlpipe( dev->udev, 0 ),
			request, requesttype,
			value, index,
			buf, size, timeout );
	if( err >= 0 && err < size )
		err = -EINVAL;
	kfree( buf );

	return 0;

err_out:
	return err;
}

static int ch9x00_mdio_read( struct net_device *netdev,
		int phy_id, int loc )
{
	struct usbnet *dev = netdev_priv(netdev);
	int product_id = dev->udev->descriptor.idProduct;
	__le16 res = 0x00; //for E091
	unsigned char buff[2]; //for E092

	dbg_prt("ch9x00_mdio_read phy_id:%02x loc:%02x\n", phy_id, loc);

	if( product_id == CH9x00_PID_E091 ||
			product_id == CH9x00_PID_8339 ) {
		mutex_lock( &dev->phy_mutex );
		if( phy_id == 0x00 ) {
			switch(loc) {
				case MII_BMCR: //Basic mode control register
				{
					if(speed_status == DEVICE_SPEED_10M){
						//Do nothing here
					}
					else
						res |= BMCR_SPEED100;

					if(duplex_status == DEVICE_DUPLEX_FULL)
						res |= BMCR_FULLDPLX;
					else {
						//Do nothing here
					}

					res |= BMCR_ANENABLE;
					break;
				}
				case MII_BMSR: //Basic mode status register
				{
					if(link_status == DEVICE_MEDIA_CONNECTED) //up
						res |= BMSR_LSTATUS;
					else { //down
						//Do nothing
					}

					if(speed_status == DEVICE_SPEED_10M) {
						if(duplex_status == DEVICE_DUPLEX_FULL)
							res |= BMSR_10FULL;
						else
							res |= BMSR_10HALF;
					}
					else {
						if(duplex_status == DEVICE_DUPLEX_FULL)
							res |= BMSR_100FULL;
						else
							res |= BMSR_100HALF;
					}

					break;
				}
				case MII_ADVERTISE: //Advertisement control reg
				{
					if(speed_status == DEVICE_SPEED_10M)
						res |= ADVERTISE_10FULL;
					else
						res |= ADVERTISE_10HALF;

					res |= 0x01; //IEEE 802.3
				}
				case MII_LPA: //Link partner ability reg
				{
					if(speed_status == DEVICE_SPEED_10M) {
						if(duplex_status == DEVICE_DUPLEX_FULL)
							res |= LPA_10FULL;
						else
							res |= LPA_10HALF;
					}
					else {
						if(duplex_status == DEVICE_DUPLEX_FULL)
							res |= LPA_100FULL;
						else
							res |= LPA_100HALF;
					}

					res |= 0x01; //IEEE 802.3
					break;
				}
				case MII_EXPANSION: //Expansion register
					break;
				default:
					break;
			}
		}
		mutex_unlock( &dev->phy_mutex);
		return le16_to_cpu(res);
	}
	else if( product_id == CH9x00_PID_E092 ) {
		if( phy_id )
			return 0;

		control_read( dev, REQUEST_READ, REQUEST_TYPE_READ, 
				0, loc*2, buff, 0x02, CONTROL_TIMEOUT_MS );

		return ( buff[0] | buff[1] << 8 );
	}

	return 0;
}


static void ch9x00_mdio_write( struct net_device *netdev,
		int phy_id, int loc, int val )
{
	struct usbnet *dev = netdev_priv(netdev);
	int product_id = dev->udev->descriptor.idProduct;
	unsigned char value_l = 0; //for E091
	unsigned char buff[2]; //for E092

	dbg_prt("ch9x00_mdio_write() phy_id=%02x loc:%02x\n", phy_id, loc);

	if( product_id == CH9x00_PID_E091 ||
			product_id == CH9x00_PID_8339 ) {
		mutex_lock( &dev->phy_mutex );
		if(phy_id == 0x00) {
			switch(loc) {
				case MII_BMCR: //Base mode control register
				{
					if( val & BMCR_ANRESTART ) {
						//Do nothing
					}
					
					if( val & BMCR_ANENABLE ) {
						value_l |= AT;
						goto set;
					}

					if( val & BMCR_SPEED100 ) {
						value_l |= SPEED_100M;
					}

					// bit 8 duplex mode 1 = full-duplex
					if( val & BMCR_FULLDPLX ) {
						value_l |= FULL_DUPLEX;
					}
set:
					set_speed_duplex(dev, value_l);
					break;
				}
				case MII_BMSR: //Basic mode status register
					break;
				case MII_ADVERTISE:	//Advertisement control reg
					break;
				case MII_LPA: //Link partner ability reg
					break;
				case MII_EXPANSION: //Expansion register
					break;
				default:
					break;
			}
		}
		mutex_unlock( &dev->phy_mutex );
		return;
	}
	else if( product_id == CH9x00_PID_E092 ) {
		buff[0] = (unsigned char)val;
		buff[1] = (unsigned char)(val >> 8);
		
		if(phy_id)
			return;

		control_write( dev, REQUEST_WRITE, REQUEST_TYPE_WRITE,
				0, loc*2, buff, 0x02, CONTROL_TIMEOUT_MS );

	}

}


static int ch9x00_link_reset( struct usbnet *dev )
{
	struct ethtool_cmd ecmd;

	mii_check_media( &dev->mii, 1, 1 );
	mii_ethtool_gset( &dev->mii, &ecmd );

	dbg_prt("\nlink_reset() speed:%d duplex:%d \n", ecmd.speed, ecmd.duplex );

	return 0;
}


static void ch9x00_status( struct usbnet *dev, struct urb *urb )
{
	int link;
	unsigned char *buf;
	int product_id = dev->udev->descriptor.idProduct;

	if( product_id == CH9x00_PID_E091 ||
			product_id == CH9x00_PID_8339 ) {
		if( urb->actual_length < 8 )
			return;

		buf = urb->transfer_buffer;
		link = !!(buf[0] & DEVICE_MEDIA_CONNECTED );
		link_status = buf[0] & DEVICE_MEDIA_CONNECTED;
		speed_status = buf[0] & DEVICE_SPEED_10M;
		duplex_status = buf[0] & DEVICE_DUPLEX_FULL;
		phy_status = buf[0] & DEVICE_PHY;

		if( netif_carrier_ok(dev->net) != link ) {
			if(link) {
				netif_carrier_on(dev->net);
				usbnet_defer_kevent(dev, EVENT_LINK_RESET);
			}
			else {
				netif_carrier_off(dev->net);
			}
		}
	}
	else if( product_id == CH9x00_PID_E092 ) {
		if( urb->actual_length < 16 ) 
			return;

		buf = urb->transfer_buffer;
		link = !!(buf[0] & 0x01);

		if( link ) {
			netif_carrier_on(dev->net);
			usbnet_defer_kevent(dev, EVENT_LINK_RESET);
		}
		else {
			netif_carrier_off(dev->net);
		}
	}
	

	return;
}


static struct sk_buff *ch9x00_tx_fixup( 
		struct usbnet *dev, struct sk_buff *skb, gfp_t flags )
{
	int i = 0;
	int len = 0;
	int tx_overhead = 0;
	int product_id = dev->udev->descriptor.idProduct;

	if( product_id == CH9x00_PID_E091 ||
			product_id == CH9x00_PID_8339 )
		tx_overhead = TX_OVERHEAD; //TX_OVERHEAD: 0x04
	else if( product_id == CH9x00_PID_E092 )
		tx_overhead = 0x40; //64

	len = skb->len;
	if( skb_headroom(skb) < tx_overhead ) {
		struct sk_buff *skb2;

		skb2 = skb_copy_expand(skb, tx_overhead, 0, flags);
		dev_kfree_skb_any(skb);
		skb = skb2;
		if( !skb )
			return NULL;
	}

	__skb_push(skb, tx_overhead);
	/* usbnet adds padding if length is a multiple of packet size 
	   if so, adjust length value in header */
	if( (skb->len % dev->maxpacket) == 0 ) {
		len++;
	}

	if( product_id == CH9x00_PID_E091 ||
			product_id == CH9x00_PID_8339 ) {
		skb->data[0] = len;
		skb->data[1] = len >> 8;
		skb->data[2] = 0x00;
		skb->data[3] = 0x00;
	}
	else if( product_id == CH9x00_PID_E092 ) {
		skb->data[0] = len;
		skb->data[1] = len >> 8;
		skb->data[2] = 0x00;
		skb->data[3] = 0x80;

		for( i = 4; i < 48; i++ )
			skb->data[i] = 0x00;

		skb->data[48] = len;
		skb->data[49] = len >> 8;
		skb->data[50] = 0x00;
		skb->data[51] = 0x80;

		for( i = 52; i < 64; i++ )
			skb->data[i] = 0x00;
	}

	return skb;
}


static int ch9x00_rx_fixup( struct usbnet *dev, struct sk_buff *skb )
{
	int len = 0;
	int rx_overhead = 0;
	int product_id = dev->udev->descriptor.idProduct;

	if( product_id == CH9x00_PID_E091 ||
			product_id == CH9x00_PID_8339 ) {
		rx_overhead = RX_OVERHEAD;

		if( unlikely(skb->len < rx_overhead) ) {
			dev_err( &dev->udev->dev, "unexpected tiny rx frame\n");
			return 0;
		}

		len = (skb->data[0] | skb->data[1] << 8 );

		skb_pull(skb, rx_overhead);
		skb_trim(skb, len);
	}
	else if( product_id == CH9x00_PID_E092 ) {
		// Do nothing here
		rx_overhead = 64;

		if( unlikely(skb->len < rx_overhead) ) {
			dev_err( &dev->udev->dev, "unexpected tiny rx frame\n");
			return 0;
		}

		len = (skb->data[skb->len - 16] | skb->data[skb->len - 15] << 8 );
		/*printk("rx_fixup skb->len=%d, len=%d\n", skb->len, len );*/

		/*skb_pull(skb, rx_overhead);*/
		skb_trim(skb, len);
	}

	return 1;
}


static int get_mac_address( struct usbnet *dev, unsigned char *data )
{
	int err = 0;
	//for E092
	unsigned char mac_addr[0x06];
	int rd_mac_len = 0;

	dbg_prt("\n--> get_mac_address:\n\tusbnet VID:%0x PID:%0x\n", 
			dev->udev->descriptor.idVendor,
			dev->udev->descriptor.idProduct);

	if( dev->udev->descriptor.idProduct == CH9x00_PID_E091 ||
			dev->udev->descriptor.idProduct == CH9x00_PID_8339 ) 
		err = control_read( dev, GET_MAC_ADDRESS, REQ_RD_REG, 
				0, 0, data, 0x06, CTRL_TIMEOUT_MS );
	else if( dev->udev->descriptor.idProduct == CH9x00_PID_E092 ) {
		memset( mac_addr, 0, sizeof(mac_addr) );
		rd_mac_len = control_read( dev, REQUEST_READ, REQUEST_TYPE_READ, 
				0, MAC_REG_STATION_L, mac_addr, 0x02, CONTROL_TIMEOUT_MS );
		rd_mac_len += control_read( dev, REQUEST_READ, REQUEST_TYPE_READ, 
				0, MAC_REG_STATION_M, mac_addr+2, 0x02, CONTROL_TIMEOUT_MS );
		rd_mac_len += control_read( dev, REQUEST_READ, REQUEST_TYPE_READ, 
				0, MAC_REG_STATION_H, mac_addr+4, 0x02, CONTROL_TIMEOUT_MS );
		if( rd_mac_len != ETH_ALEN )
			err = -EINVAL;
		//Set MAC
		data[0] = mac_addr[5];
		data[1] = mac_addr[4];
		data[2] = mac_addr[3];
		data[3] = mac_addr[2];
		data[4] = mac_addr[1];
		data[5] = mac_addr[0];
	}

	if( err < 0 )
		goto err_out;

		return 0;

err_out:
	return err;
}


static int ch9x00_bind( struct usbnet *dev, struct usb_interface *intf )
{
	int retval = 0;
	unsigned char data[2];

	//int vendor_id  = dev->udev->descriptor.idVendor;
	int product_id = dev->udev->descriptor.idProduct;

	retval = usbnet_get_endpoints(dev, intf);
	if(retval)
		goto err_out;

	// compatibility of E091 and E092
	if( product_id == CH9x00_PID_E091 ||
			product_id == CH9x00_PID_8339 ) {
		if(	(retval = get_mac_address(dev, dev->net->dev_addr)) < 0 ) {
			return retval;
			goto err_out;
		}
	}

	// Initialize MII structure for Setting Speed Duplex
	dev->mii.dev = dev->net;
	dev->mii.mdio_read = ch9x00_mdio_read;
	dev->mii.mdio_write = ch9x00_mdio_write;
	dev->mii.reg_num_mask = 0x1f;

	if( product_id == CH9x00_PID_E091 ||
			product_id == CH9x00_PID_8339 ) {
		dev->mii.phy_id_mask = 0x3f;
		/*dev->rx_urb_size = dev->net->mtu + 14 + RX_OVERHEAD;*/
		// Note: Max package length=1518  (MTU + ETH_HELN + RX_OVERHEAD)
		dev->rx_urb_size = dev->net->mtu + ETH_HLEN + RX_OVERHEAD; 
		// Adaptor can restart, when driver rmmoded or insmoded
		mii_nway_restart( &dev->mii );
	}
	else if( product_id == CH9x00_PID_E092 ) {
		dev->mii.phy_id_mask = 0x1f;

		dev->hard_mtu = dev->net->mtu + dev->net->hard_header_len;
		// Note: Max package legth= 24*64 + 16 
		dev->rx_urb_size = 24*64 + 16;
		// Adaptor can restart, when driver rmmoded or insmoded
		mii_nway_restart( &dev->mii );

		// Initilization hardware
		data[0] = 0x01;
		data[1] = 0x0F;
		retval = control_write( dev, REQUEST_WRITE, REQUEST_TYPE_WRITE,
				0, 88, data, 0x02, CONTROL_TIMEOUT_MS );

		data[0] = 0xA0;
		data[1] = 0x90;
		retval = control_write( dev, REQUEST_WRITE, REQUEST_TYPE_WRITE,
				0, 90, data, 0x02, CONTROL_TIMEOUT_MS );

		data[0] = 0x30;
		data[1] = 0x00;
		retval = control_write( dev, REQUEST_WRITE, REQUEST_TYPE_WRITE,
				0, 92, data, 0x02, CONTROL_TIMEOUT_MS );

		data[0] = 0x17;
		data[1] = 0xD8;
		retval = control_write( dev, REQUEST_WRITE, REQUEST_TYPE_WRITE,
				0, 94, data, 0x02, CONTROL_TIMEOUT_MS );

		data[0] = 0x01;
		data[1] = 0x00;
		retval = control_write( dev, REQUEST_WRITE, REQUEST_TYPE_WRITE,
				0, 254, data, 0x02, CONTROL_TIMEOUT_MS );

		data[0] = 0x5F; // 0x0101 1111
		data[1] = 0x0D; // 0x0000 1101
		// Control Register Setting
		retval = control_write( dev, REQUEST_WRITE, REQUEST_TYPE_WRITE, 
				0, 64, data, 0x02, CONTROL_TIMEOUT_MS );

		if(	(retval = get_mac_address(dev, dev->net->dev_addr)) < 0 ) {
			return retval;
			goto err_out;
		}
	}
	if( retval < 0 )
		goto err_out;

	return 0;

err_out:
	return retval;
}


static const struct driver_info ch9x00_info = {
	.description = "CH9x00 USB to Network Adaptor",
	.flags       = FLAG_ETHER,
	.bind		 = ch9x00_bind,
	.rx_fixup    = ch9x00_rx_fixup,
	.tx_fixup    = ch9x00_tx_fixup,
	.status		 = ch9x00_status,
	.link_reset  = ch9x00_link_reset,
	.reset       = ch9x00_link_reset,
};

static const struct usb_device_id ch9x00_products[] = {
	{
		USB_DEVICE( 0x1A86, 0xE091 ),
		.driver_info = (unsigned long)&ch9x00_info,
	},
	{
		USB_DEVICE( 0x1A86, 0xE092 ),
		.driver_info = (unsigned long)&ch9x00_info,
	},
	{	USB_DEVICE( 0x1A86, 0x8339 ),
		.driver_info = (unsigned long)&ch9x00_info,
	},
	{},
};
MODULE_DEVICE_TABLE( usb, ch9x00_products );

static struct usb_driver ch9x00_driver = {
	.name = "ch9x00",
	.id_table = ch9x00_products,
	.probe = usbnet_probe,
	.disconnect = usbnet_disconnect,
	.suspend = usbnet_suspend,
	.resume = usbnet_resume,
};

static int __init ch9x00_init( void )
{
	printk( KERN_INFO "\tCH9x00 Driver Version:%s\n", DRIVER_VERSION);
	return usb_register( &ch9x00_driver );
}

static void __exit ch9x00_exit( void )
{
	usb_deregister( &ch9x00_driver );
}

module_init( ch9x00_init );
module_exit( ch9x00_exit );

MODULE_DESCRIPTION( "USB to Network adapter CH9x00" );
MODULE_LICENSE( "GPL" );
