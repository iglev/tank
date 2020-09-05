#ifndef LIBIPC_NETPACK_H_
#define LIBIPC_NETPACK_H_

#include "base/typedef.h"
#include "base/safe_singleton.h"
#include "ipc/ipc_config.h"
#include <boost/noncopyable.hpp>
#include <atomic>

NamespaceStart

class EndianConver
{
public:
	explicit EndianConver()
	{
		uint16 zn_x = 0x1234;
		uint8 zn_y = *((uint8*)(&zn_x));
		if (zn_y == 0x12)
		{
			// big
			mb_local_little_endian = false;
		}
		else
		{
			// little
			mb_local_little_endian = true;
		}
	}

	~EndianConver() = default;

public:
	void* to_net_buff(void* pp_buff, uint32 pn_len)
	{
		if (!mb_local_little_endian)
		{
			return pp_buff;	//本地是大端不需要转换
		}

		ubyte tmp = 0;
		ubyte* tmpbuff = (ubyte*)pp_buff;
		for (uint32 i = 0; i < pn_len / 2; ++i)
		{
			tmp = tmpbuff[i];
			tmpbuff[i] = tmpbuff[pn_len - 1 - i];
			tmpbuff[pn_len - 1 - i] = tmp;
		}
		return pp_buff;
	}

	void* to_host_buff(void* pp_buff, uint32 pn_len)
	{
		if (!mb_local_little_endian)
		{
			return pp_buff;	//本地是大端不需要转换
		}

		ubyte tmp = 0;
		ubyte* tmpbuff = (ubyte*)pp_buff;
		for (uint32 i = 0; i < pn_len / 2; ++i)
		{
			tmp = tmpbuff[i];
			tmpbuff[i] = tmpbuff[pn_len - 1 - i];
			tmpbuff[pn_len - 1 - i] = tmp;
		}
		return pp_buff;
	}

private:
	bool mb_local_little_endian;
};


/**************************************************
*内存结构
*| size | id | sid | type | oldlen | strdata
*--------------------------------------------------
*| 2    | 4  | 4   | 1    | 2      | xxxxx
*--------------------------------------------------
* size:    协议包长度
* id:      协议id
* sid:     业务id(校验码,...)
* type:    strdata的数据格式(msgpack, lz4, ...)
* oldlen:  原始数据长度
* strdata: 数据
*/
template<uint64 MAX_VALUE, uint32 HEAD_LEN>
class NetPacket : boost::noncopyable
{
public:
	explicit NetPacket(uint16 pn_init_size = 16)
		: mn_change_pos(0)
		, mn_buff_size(0)
		, mp_buff(nullptr)
	{
		expand_buff(pn_init_size > MAX_VALUE ? MAX_VALUE : pn_init_size);
	}

	explicit NetPacket(const void* pv_netbuff, uint16 pn_buff_len)
		: mn_change_pos(0)
		, mn_buff_size(0)
		, mp_buff(nullptr)
	{
		if (pv_netbuff == nullptr || pn_buff_len < HEAD_LEN)
		{
			expand_buff(0);
		}
		else
		{
			mn_buff_size = pn_buff_len;
			mn_change_pos = HEAD_LEN;
			try
			{
				mp_buff = new ubyte[mn_buff_size];
			}
			catch (...)
			{
				safeDeleteArray(mp_buff);
				mn_buff_size = 0;
				mn_change_pos = 0;
				return;
			}

			::memcpy(mp_buff, pv_netbuff, pn_buff_len);
			if (get_buff_head() != data_len())
			{
				reset(); // invalid data
			}
		}
	}

	~NetPacket()
	{
		safeDeleteArray(mp_buff);
	}

	bool empty() const
	{
		return (data_len() == 0);
	}

	ubyte* data()
	{
		return empty() ? nullptr : (mp_buff + mn_change_pos);
	}

	uint16 data_len() const
	{
		return (mn_buff_size - mn_change_pos);
	}

	// get_buff_head() == data_len()
	uint16 get_buff_head()
	{
		ubyte* head_buff = (mp_buff + mn_change_pos) - HEAD_LEN;
		uint16 head_value = 0;
		::memcpy(&head_value, head_buff, HEAD_LEN);
		mo_conver.to_host_buff(&head_value, HEAD_LEN);
		return head_value;
	}

	ubyte* buff()
	{
		return ((mp_buff + mn_change_pos) - HEAD_LEN);
	}

	uint16 buff_len() const
	{
		return HEAD_LEN + data_len();
	}

	uint16 maxsize() const
	{
		return mn_buff_size;
	}

	void reset()
	{
		mn_change_pos = mn_buff_size;
	}

	/*	往协议包加入数据。
	*	@param pv_data, 数据包的指针。
	*	@param pn_len, 数据包的长度。
	*   @param pb_conver, 是否需要转换成网络字节序
	*	@return 成功加入协议内存的数据包大小。
	*/
	uint16 push(const void* pv_data, uint16 pn_len, bool pb_conver = true)
	{
		if (pv_data == nullptr || pn_len <= 0)
		{
			return 0;
		}

		if (!expand_buff(pn_len))
		{
			return 0;
		}

		mn_change_pos -= pn_len;
		::memcpy(mp_buff + mn_change_pos, pv_data, pn_len);
		if (pb_conver)
		{
			mo_conver.to_net_buff(mp_buff + mn_change_pos, pn_len);
		}
		set_buff_head();

		return pn_len;
	}

	/*	从协议包取出数据。
	*	@param pv_data, 数据包的指针。
	*	@param len, 数据包的长度。
	*   @param pb_conver, 是否需要转换成本地字节序
	*	@return 成功从协议内存取出的数据包大小。
	*/
	uint16 pop(void* pv_data, uint16 pn_len, bool pb_conver = true)
	{
		if (pv_data == nullptr || pn_len <= 0)
		{
			return 0;
		}

		if (data_len() < pn_len)
		{
			return 0;
		}

		::memcpy(pv_data, mp_buff + mn_change_pos, pn_len);
		if (pb_conver)
		{
			mo_conver.to_host_buff(pv_data, pn_len);
		}
		mn_change_pos += pn_len;
		set_buff_head();

		return pn_len;
	}

	/*	push 的辅助操作。 */
	uint16 push_bool(bool val) { return push(&val, sizeof(bool)); }
	uint16 push_int8(int8 val) { return push(&val, sizeof(int8)); }
	uint16 push_int16(int16 val) { return push(&val, sizeof(int16)); }
	uint16 push_int32(int32 val) { return push(&val, sizeof(int32)); }
	uint16 push_int64(int64 val) { return push(&val, sizeof(int64)); }
	uint16 push_uint8(uint8 val) { return push(&val, sizeof(uint8)); }
	uint16 push_uint16(uint16 val) { return push(&val, sizeof(uint16)); }
	uint16 push_uint32(uint32 val) { return push(&val, sizeof(uint32)); }
	uint16 push_uint64(uint64 val) { return push(&val, sizeof(uint64)); }
	uint16 push_float(float val) { return push(&val, sizeof(float)); }
	uint16 push_double(double val) { return push(&val, sizeof(double)); }
	uint16 push_string(const std::string& val)
	{
		uint16 len = 0;
		uint16 str_len = (uint16)val.size();
		if (str_len > 0)
		{
			len += push(val.data(), str_len, false);
		}
		len += push_uint16(str_len);

		return len;
	}

	/*	pop 的辅助操作。 */
	uint16 pop_bool(bool* val) { return pop(val, sizeof(bool)); }
	uint16 pop_int8(int8* val) { return pop(val, sizeof(int8)); }
	uint16 pop_int16(int16* val) { return pop(val, sizeof(int16)); }
	uint16 pop_int32(int32* val) { return pop(val, sizeof(int32)); }
	uint16 pop_int64(int64* val) { return pop(val, sizeof(int64)); }
	uint16 pop_uint8(uint8* val) { return pop(val, sizeof(uint8)); }
	uint16 pop_uint16(uint16* val) { return pop(val, sizeof(uint16)); }
	uint16 pop_uint32(uint32* val) { return pop(val, sizeof(uint32)); }
	uint16 pop_uint64(uint64* val) { return pop(val, sizeof(uint64)); }
	uint16 pop_float(float* val) { return pop(val, sizeof(float)); }
	uint16 pop_double(double* val) { return pop(val, sizeof(double)); }
	uint16 pop_string(std::string* val)
	{
		val->clear();
		uint16 str_len = 0;
		uint16 tmp = pop_uint16(&str_len);
		if (tmp <= 0) return 0;		//没有uint16长度

		if (str_len <= 0) return tmp;	//字符串长度为0

		char* zs_tmp_str = new char[str_len + 1];
		uint16 tmp2 = pop(zs_tmp_str, str_len, false);
		if (tmp2 <= 0)
		{
			safeDeleteArray(zs_tmp_str);
			return 0;
		}
		zs_tmp_str[str_len] = 0;
		val->assign(zs_tmp_str, str_len);
		safeDeleteArray(zs_tmp_str);
		return str_len + tmp;
	}

private:
	void set_buff_head()
	{
		ubyte* head_buff = mp_buff + (mn_change_pos - HEAD_LEN);
		uint16 datalen = data_len();
		::memcpy(head_buff, &datalen, HEAD_LEN);
		mo_conver.to_net_buff(head_buff, HEAD_LEN);
	}

	bool expand_buff(uint16 pn_init_size)
	{
		if (pn_init_size >= MAX_VALUE)
		{
			return false;
		}

		if (mp_buff == nullptr)
		{
			// first create
			mn_buff_size = HEAD_LEN + pn_init_size;
			mn_change_pos = mn_buff_size;
			try
			{
				mp_buff = new ubyte[mn_buff_size];
			}
			catch (...)
			{
				safeDeleteArray(mp_buff);
				mn_buff_size = 0;
				mn_change_pos = 0;
				return false;
			}
			::memset(mp_buff, 0, mn_buff_size);
		}
		else if ((pn_init_size == 0) || ((mn_buff_size - buff_len()) > pn_init_size))
		{
			// space not enough
		}
		else if ((MAX_VALUE - buff_len()) >= pn_init_size)
		{
			// expand
			ubyte* old_buff = buff();
			uint16 old_buff_len = buff_len();

			uint32 new_buff_size = uint32(mn_buff_size) * 2 + pn_init_size;
			if (new_buff_size > MAX_VALUE)
			{
				new_buff_size = MAX_VALUE;
			}

			ubyte* new_buff = nullptr;
			try
			{
				new_buff = new ubyte[new_buff_size];
			}
			catch (...)
			{
				safeDeleteArray(new_buff);
				return false;
			}

			::memcpy(new_buff + (new_buff_size - old_buff_len), old_buff, old_buff_len);
			mn_buff_size = (uint16)new_buff_size;
			mn_change_pos = (uint16)(new_buff_size - old_buff_len + HEAD_LEN);
			safeDeleteArray(mp_buff);
			mp_buff = new_buff;
		}
		else
		{
			// too big
			return false;
		}

		return true;
	}

private:
	uint16 mn_change_pos;
	uint16 mn_buff_size;
	ubyte* mp_buff;
	EndianConver mo_conver;
};
typedef NetPacket<((1 << 16) - 1), 2> NetPacket2;

class RecvBuffer : boost::noncopyable
{
public:
	explicit RecvBuffer(uint32 pn_init_size = 1024)
		: mn_push_pos(0)
		, mn_pop_pos(0)
		, mp_buff(nullptr)
	{
		if (pn_init_size > (1 << 20))
		{
			pn_init_size = (1 << 20);  // 1M为上限
		}

		mn_buff_size = pn_init_size;
		try
		{
			mp_buff = new ubyte[mn_buff_size];
		}
		catch (...)
		{
			mn_buff_size = 0;
			safeDeleteArray(mp_buff);
		}
	}

	~RecvBuffer()
	{
		safeDeleteArray(mp_buff);
	}

	bool push(void* pv_buff, uint32 pn_len)
	{
		if (pv_buff == nullptr || pn_len <= 0)
		{
			return false;
		}

		if (!make_have_buffer(pn_len))
		{
			return false;
		}

		::memcpy(mp_buff + mn_push_pos, pv_buff, pn_len);
		mn_push_pos += pn_len;
		return true;
	}

	bool pop(uint32 pn_len)
	{
		if (pn_len <= 0 || get_used_size() < pn_len)
		{
			return false;
		}
		mn_pop_pos += pn_len;
		return true;
	}

	void* front(uint32 pn_len)
	{
		if (pn_len <= 0 || get_used_size() < pn_len)
		{
			return 0;
		}

		return mp_buff + mn_pop_pos;
	}

	bool full() const
	{
		return (get_used_size() == mn_buff_size);
	}

	bool empty() const
	{
		return (get_used_size() == 0);
	}

	uint32 get_used_size() const
	{
		return mn_push_pos - mn_pop_pos;
	}

	uint32 get_free_size() const
	{
		return (mn_buff_size - get_used_size());
	}

	uint32 get_max_size() const
	{
		return mn_buff_size;
	}

private:
	bool make_have_buffer(uint32 pn_len)
	{
		uint32 zn_need_size = pn_len;
		uint32 zn_left_size = mn_buff_size - mn_push_pos;
		if (zn_left_size >= zn_need_size)
		{
			// tail space enough
			return true;
		}
		else if (get_free_size() >= zn_need_size)
		{
			// space enough, but need move
			uint32 zn_used_size = get_used_size();
			if (zn_used_size > 0)
			{
				for (uint32 i = 0; i < zn_used_size; ++i)
				{
					mp_buff[i] = mp_buff[i + mn_pop_pos];
				}
			}
			mn_pop_pos = 0;
			mn_push_pos = zn_used_size;
			return true;
		}
		else
		{
			// realloc
			uint32 zn_new_buff_size = uint32(mn_buff_size * 1.5) + zn_need_size;
			ubyte* zp_new_buff = nullptr;
			try
			{
				zp_new_buff = new ubyte[zn_new_buff_size];
			}
			catch (...)
			{
				safeDeleteArray(zp_new_buff);
				return false;
			}
			uint32 zn_used_size = get_used_size();
			if (zn_used_size > 0)
			{
				::memcpy(zp_new_buff, mp_buff + mn_pop_pos, zn_used_size);
			}
			safeDeleteArray(mp_buff);
			mn_pop_pos = 0;
			mn_push_pos = zn_used_size;
			mn_buff_size = zn_new_buff_size;
			mp_buff = zp_new_buff;

			return true;
		}
	}

private:
	//始位置  
	uint32 mn_push_pos;
	//尾位置  
	uint32 mn_pop_pos;

	//缓冲区大小  
	uint32 mn_buff_size;
	//缓冲区指针  
	ubyte* mp_buff;
};

class RecvPacketBuilder : boost::noncopyable
{
public:
	explicit RecvPacketBuilder(uint32 pn_init_size = 1024)
		: mb_reading_head(true)
		, mn_recv_head(0)
		, mn_errcode(0)
		, mo_buff(pn_init_size)
	{

	}

	~RecvPacketBuilder() = default;

	uint32 push(void* pv_buff, uint32 pn_len)
	{
		mn_errcode = 0;
		if (pv_buff != 0 && pn_len > 0)
		{
			mo_buff.push(pv_buff, pn_len);
		}

		bool had_data = true;
		while (had_data)
		{
			if (mb_reading_head)	//读头
			{
				void* pbuff = mo_buff.front(PACK_HEAD_LEN);
				had_data = (pbuff != 0);
				if (had_data)
				{
					memcpy(&mn_recv_head, pbuff, PACK_HEAD_LEN);
					zo_conver.to_host_buff(&mn_recv_head, PACK_HEAD_LEN);
					mb_reading_head = !mb_reading_head;
				}
				if (had_data && mn_recv_head > PACK_MAX_RECV_LEN)	//超出限定大小
				{
					mn_errcode = -1;
					return 0;
				}
			}
			else	//读数据
			{
				uint32 datalen = PACK_HEAD_LEN + mn_recv_head;
				void* pbuff = mo_buff.front(datalen);
				had_data = (pbuff != 0);

				if (had_data)
				{
					mb_reading_head = !mb_reading_head;
					return datalen;
				}
			}
		}
		return 0;
	}

	bool pop(uint32 pn_len)
	{
		return mo_buff.pop(pn_len);
	}

	void* front(uint32 pn_len)
	{
		return mo_buff.front(pn_len);
	}

	int32 error_code() const
	{
		return mn_errcode;
	}

private:
	bool mb_reading_head;
	uint16 mn_recv_head;
	int32 mn_errcode;
	RecvBuffer mo_buff;
	EndianConver zo_conver;
};

NamespaceEnd



#endif





















