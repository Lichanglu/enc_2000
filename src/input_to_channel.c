/********************************************************************
*  专门用来封装channel ,input,select link chid,enclink chid 的对应关系
*
*
*
*
*********************************************************************/

#include <stdio.h>






#include "reach_enc2000.h"
#include "input_to_channel.h"
#include "log_common.h"
/*
*		根据实际的代码,4路channel对应的enclink里面的input chid的顺序可能不一致，
*		所以封装找个函数
*/
static int g_enc_chid[MAX_CHANNEL] = {1, 0, 3, 2, 4, 5};
//form CHANNEL_INPUT_N to enclik chid
int channel_get_enc_chid(int channel)
{
	//	int enc_chid = 0;

	if(channel < 0 || channel > MAX_CHANNEL) {
		ERR_PRN("channel =%d is invaild\n", channel);
		return -1;
	}

	return g_enc_chid[channel];
}

//from enclink chid to channel_input_N
int enc_chid_get_channel(int enc_chid, int mp_status)
{
	int i = 0;

	if(IS_IND_STATUS == mp_status) {
		for(i = 0 ; i <= CHANNEL_INPUT_2_LOW; i++) {
			if(enc_chid == g_enc_chid[i]) {
				return i;
			}
		}
	} else if(IS_MP_STATUS == mp_status) {

		for(i = CHANNEL_INPUT_MP ; i <= CHANNEL_INPUT_MP_LOW; i++) {
			if(enc_chid == g_enc_chid[i]) {
				return i;
			}
		}
	}

	ERR_PRN("enc_chid=%d is invaid\n", enc_chid);
	return -1;
}

int channel_set_enc_chid(int channel, int enc_chid)
{

	if(channel < 0 || channel > MAX_CHANNEL) {
		ERR_PRN("channel =%d is invaild\n", channel);
		return -1;
	}

	g_enc_chid[channel] = enc_chid;
	return 0;
}

int input_get_high_channel(int input)
{
	if(SIGNAL_INPUT_1 == input) {
		return CHANNEL_INPUT_1;
	} else if(SIGNAL_INPUT_2 == input) {
		return  CHANNEL_INPUT_2;
	} else if(SIGNAL_INPUT_MP == input) {
		return CHANNEL_INPUT_MP;
	} else {
		return CHANNEL_INPUT_1;
	}
}

//传入channel ,获取vp 以及high low
int channel_get_input_info(int channel, int *input , int *high)
{
	//	PRINTF("---channel=%d---\n", channel);

	switch(channel) {
		case CHANNEL_INPUT_1:
			*input = SIGNAL_INPUT_1;
			*high = HIGH_STREAM;
			break;

		case CHANNEL_INPUT_1_LOW:
			*input = SIGNAL_INPUT_1;
			*high = LOW_STREAM;
			break;

		case CHANNEL_INPUT_2:
			*input = SIGNAL_INPUT_2;
			*high = HIGH_STREAM;
			break;

		case CHANNEL_INPUT_2_LOW:
			*input = SIGNAL_INPUT_2;
			*high = LOW_STREAM;
			break;

		case CHANNEL_INPUT_MP:
			*input = SIGNAL_INPUT_MP;
			*high = HIGH_STREAM;
			break;

		case CHANNEL_INPUT_MP_LOW:
			*input = SIGNAL_INPUT_MP;
			*high = LOW_STREAM;
			break;

		default:
			PRINTF("WARNING:channle =%d\n", channel);
			*input = -1;
			*high = -1;
			break;
	}

	/*页面没做，所以强转SIGNAL_INPUT_MP

	if( IS_MP_STATUS == get_mp_status()){
		*input = SIGNAL_INPUT_MP;
	}
	*/

	return 0;
}


//mergeLink 1输出后，ch对应的最原始的几个信号输入
static int g_input1_mer1_chid = 0;
static int g_input2_mer1_chid = 1;
static int g_mp_chid = 2;

int input_set_mer1_chid(int input, int chid)
{
	if(input == SIGNAL_INPUT_1) {
		g_input1_mer1_chid = chid;
	} else if(input == SIGNAL_INPUT_2) {
		g_input2_mer1_chid = chid;
	} else if(input == SIGNAL_INPUT_MP) {
		g_mp_chid = chid;
	} else {
		WARN_PRN("input =%d is invaild\n", input);
		return -1;
	}

	PRINTF("i will set input %d to mer1 link chid =%d\n", input, chid);
	return 0;
}

/*input get mer1 link chid */
int input_get_mer1_chid(int input)
{
	if(input == SIGNAL_INPUT_1) {
		return g_input1_mer1_chid;
	} else if(input == SIGNAL_INPUT_2) {
		return g_input2_mer1_chid;
	} else if(input == SIGNAL_INPUT_MP) {
		return g_mp_chid;
	} else {
		WARN_PRN("input =%d is invaild\n", input);
		return -1;
	}
}

/*channel get mer1 link chid */
int channel_get_mer1_chid(int channel)
{
	int input = -1;
	int high = -1;
	channel_get_input_info(channel, &input , &high);

	if(input < SIGNAL_INPUT_1 || high != HIGH_STREAM) {
		WARN_PRN("input =%d,high=%d is not support.\n", input, high);
		return -1;
	}

	return input_get_mer1_chid(input);

}

static int s_audio_input = AUDIO_LINEIN_1;
int input_get_audio_input(int input, int *audio_input)
{
	int mp_status = get_mp_status();
	int ret = -1;

	if(input == SIGNAL_INPUT_MP && mp_status == IS_MP_STATUS) {
		*audio_input =  s_audio_input;
		ret = 0;
	}

	return ret;
}



int mp_set_audio_input(int audio_input, int mp_status)
{
	int ret = -1;
	PRINTF("audio_input=%d\n", audio_input);

	if(mp_status == IS_MP_STATUS) {
		s_audio_input = audio_input;
		ret = 0;
	}

	return ret;
}


int input_set_audio_input(int audio_input)
{
	int mp_status = get_mp_status();
	int ret = -1;
	PRINTF("audio_input=%d\n", audio_input);

	if(mp_status == IS_MP_STATUS) {
		s_audio_input = audio_input;
		ret = 0;
	}

	return ret;
}

int audio_input_get_input(int audio_input)
{
	int mp_status = get_mp_status();
	int input = SIGNAL_INPUT_1;

	if(IS_MP_STATUS == mp_status) {
		input = SIGNAL_INPUT_MP;
	} else {
		if(audio_input == AUDIO_LINEIN_1) {
			input = SIGNAL_INPUT_1;
		} else if(audio_input == AUDIO_LINEIN_2) {
			input = SIGNAL_INPUT_2;
		}
	}

	return input;
}

#ifndef	MAX_COM_NUM
#define	MAX_COM_NUM	4
#endif
int port_com_index[MAX_COM_NUM] = {2, 0, 1, 3};
int input_set_remote(int input, int index)
{
	if(0 > input || MAX_COM_NUM < input) {
		return -1;
	}

	printf("[input_set_remote] input:[%d] index:[%d] port_com_index:[%d]\n",
	       input, index, port_com_index[input]);
	port_com_index[input] = index;
	return port_com_index[input];
}

int input_get_remote(int input)
{
	if(0 > input || MAX_COM_NUM < input) {
		return -1;
	}

	printf("[input_get_remote] input:[%d] port_com_index:[%d]\n",
	       input, port_com_index[input]);
	return port_com_index[input];
}

int get_mp_input(int *input)
{
	int mp_status =  get_mp_status();

	if(input == NULL) {
		ERR_PRN("arg is NULL\n");
		return -1;
	}

	if(mp_status == IS_MP_STATUS) {
		*input = SIGNAL_INPUT_MP;
	}

	return 0;
}

int check_input_valid(int input)
{
	int mp_status = get_mp_status();

	if(input < 0 || input >= SIGNAL_INPUT_MAX) {
		//PRINTF("\n****input  too big!****\n");
		return 0;
	}

	if(mp_status == IS_IND_STATUS) {
		if(input != SIGNAL_INPUT_1 && input != SIGNAL_INPUT_2) {
			//PRINTF("\n****input  exceed!****\n");
			return 0;//false
		}
	} else if(mp_status == IS_MP_STATUS && input != SIGNAL_INPUT_MP) {
		//PRINTF("\n****input  too small****!!\n");
		return 0;
	}

	return 1;//true
}


