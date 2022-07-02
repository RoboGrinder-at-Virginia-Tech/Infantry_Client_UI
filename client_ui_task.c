/*************************************************************
client_ui_task.c �� client_ui_task.h ʵ���� RMUL �����ֿͻ��� �Զ���UI

����������; RoboGrinder at Virginia Tech
-----------------------------------------------------------------------------
|client_ui_task.h�еĲ��ֽṹ��ͺ궨�� 
|�� client_ui_task.c���º���:
|void UI_Delete(...);
|void Line_Draw(...);
|int UI_ReFresh(int cnt,...);
|unsigned char Get_CRC8_Check_Sum_UI(...);
|uint16_t Get_CRC16_Check_Sum_UI(...);
|void Circle_Draw(...);
|void Rectangle_Draw(...);
|void Float_Draw(...);
|void Char_Draw(...);
|int Char_ReFresh(...);
|void Arc_Draw(...);
|����:
|RM�Զ���UIЭ��       ����RM2020ѧ������ͨ��Э��V1.1
|ɽ������ѧ ����ս�� ����@Rjgawuie
|RM��̳ԭ��: https://bbs.robomaster.com/forum.php?mod=viewthread&tid=11924
|�ǳ���лɽ������ѧͬѧ�ǵķ���!
-----------------------------------------------------------------------------
���������һЩ�µĹ����������� ���������̳ԭ�����ᵽ�� "�޷��ڿͻ��˽����ϻ����ַ��͸���������"������
UI_ReFresh(..); Char_ReFresh(..); Delete_ReFresh(..); ��������: �Զ�ͨ������ϵͳ�û����� ��̬�Ļ�ȡ��ǰ������ID ��ȷ��Ŀ������ֿͻ���ID;

��ϸ��ʹ�ú�˵����readme.md

��̬��UI, ����׼��, ͼ�β������� "����", ����(UI_Graph_ADD)
��̬��UI, �糬�����ݵ�ѹ, �����̵߳�while��ʼǰ�� ����(UI_Graph_ADD)һ��, Ȼ����while����Ҫ �޸�(UI_Graph_Change) ����1HZ��Ƶ�� ����(UI_Graph_ADD) һ�ε�ǰ���µĶ�̬UI
	��������Ŀ����Ϊ�˱�֤ ���۲����ֿͻ��˻����·����� ��ʲô����; ���� �����ֿͻ��˺����·����� ��Ҫ�ڻ����ϵ�֮������; ��1HZ��Ƶ�� ����(UI_Graph_ADD) ������֤�˶�̬UI��һֱ�ȶ���ʾ
	Ҳ����˵, �������ڲ����ֿͻ����޷� "���(Clear)", ֻ��ͨ�� "����(Hide)"���ر��Զ���UI����ʾ
�������ݵ���ʾ���������Ͻ�, ������ŵ��м�, ��������д���м���ʾ�������ݵ�ѹ�Ͱٷֱ�(ע�͵���)

�� void client_ui_task(void const *pvParameters) ����ΪRTOS(ʹ��FreeRTOS)�߳�, �������´���
osThreadDef(UI, client_ui_task, osPriorityNormal, 0, 512);
client_ui_task_handle =  osThreadCreate(osThread(UI), NULL);

void ui_coord_update(): ��һЩ���������ļ���ȫ�ֱ�����FSM����Ҫ�����޸�
void UI_SendByte(unsigned char ch): ��������ӳ��, Ҳ�������޸�Ϊʹ�õİ弶����

������κ�������޸������ӭ��ϵ: zshen25@vt.edu
��΢��: wxid_0217pg5fk4y711
**************************************************************/


#include "client_ui_task.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include "SuperCap_comm.h"
#include "shoot.h"
#include "referee.h"
#include "referee_usart_task.h"
#include "detect_task.h"

#if INCLUDE_uxTaskGetStackHighWaterMark
uint32_t client_ui_task_high_water;
#endif

extern uint8_t turboMode;
extern uint8_t swing_flag;

extern shoot_control_t shoot_control; 
extern miniPC_info_t miniPC_info;
extern supercap_can_msg_id_e current_superCap;
extern wulieCap_info_t wulie_Cap_info;

unsigned char UI_Seq;     //�����

//����ͼ������ SZL ������ ��̬��׼��
Graph_Data gAimVertL, gAimHorizL1m, gAimHorizL2m, gAimHorizL4m, gAimHorizL5m, gAimHorizL7m, gAimHorizL8m, left8to7, left7to5,left5to4,left4to2, right8to7, right7to5,right5to4,right4to2;

String_Data strChassisSts, strSPINSts;//���Ͻ� deleted word strCapVolt, strCapPct,
String_Data strCVSts, strGunSts, strABoxSts, strProjSLimSts, strDisSts;//���Ͻ�

//��̬�Ķ��� ��ѹ
Float_Data fCapVolt, fCapPct;
Float_Data fProjSLim, fDis; //���Ͻ�

//����
Graph_Data gChassisSts_box, gSPINSts_box;//���Ͻ�
Graph_Data gCVSts_box, gGunSts_box, gABoxSts_box;//���Ͻ�
Graph_Data gEnemyDetected_circle, gCVfb_sts_box;

//ɾ��ͼ��ṹ��
//UI_Data_Delete delLayer;

//UI��Ϣ��װ�ṹ��
ui_info_t ui_info;

uint32_t client_ui_count_ref = 0;
uint8_t client_ui_test_flag = 1;

uint32_t ui_dynamic_crt_send_TimeStamp;
const uint16_t ui_dynamic_crt_sendFreq = 1000;

void client_ui_task(void const *pvParameters)
{
	//�ȴ�referee_usart_task����ɶ�MCU UART6�ĳ�ʼ��
	vTaskDelay(200);

	//���ٻ���"����"һ�ζ�̬UI
	//��̬ͼ�� ռ��ͼ�� 4,5,6,7
			
	//���Ͻ� �������ݵ�ѹ
	//����ʱsuperCap_info.VBKelvin_fromCap�޸�Ϊ 0
	Float_Draw(&fCapVolt, "999", UI_Graph_ADD, 4, UI_Color_Yellow, 20, 2, 3, 1620, 810, 00.00);//superCap_info.VBKelvin_fromCap);
	//new position of voltage of sup_cap ��׼���м�
	//Float_Draw(&fCapVolt, "999", UI_Graph_ADD, 4, UI_Color_Yellow, 20, 2, 3, 960-20, 200, 00.00);//superCap_info.VBKelvin_fromCap);
	//���Ͻ� �������ݰٷֱ�
	//����ʱsuperCap_info.EBPct_fromCap�޸�Ϊ 0
	Float_Draw(&fCapPct, "998", UI_Graph_ADD, 4, UI_Color_Yellow, 20, 2, 3, 1620, 840, 00.00);//superCap_info.EBPct_fromCap);
	//new display position with number size = 40
	//Float_Draw(&fCapPct, "998", UI_Graph_ADD, 4, UI_Color_Yellow, 40, 2, 3, 960-20, 250, 00.00);//superCap_info.EBPct_fromCap);
	//new position of rectangle ���ϽǷ���
	Rectangle_Draw(&gChassisSts_box, "997", UI_Graph_ADD, 4, UI_Color_Cyan, 3, TopRight_REC_on_NORM_START_X-80, TopRight_REC_on_NORM_START_Y-95, TopRight_REC_on_NORM_END_X-80, TopRight_REC_on_NORM_END_Y-95);
	Rectangle_Draw(&gSPINSts_box, "996", UI_Graph_ADD, 4, UI_Color_Cyan, 3, TopRight_REC_on_FOLL_START_X-80, TopRight_REC_on_FOLL_START_Y-95, TopRight_REC_on_FOLL_END_X-80, TopRight_REC_on_FOLL_END_Y-95);
	
	//���Ͻ�---��δ��
	Rectangle_Draw(&gCVSts_box, "995", UI_Graph_ADD, 4, UI_Color_Cyan, 3, 240, 850, 330, 815);
	Rectangle_Draw(&gGunSts_box, "994", UI_Graph_ADD, 4, UI_Color_Cyan, 3, 240, 810, 330, 775);
	Rectangle_Draw(&gABoxSts_box, "993", UI_Graph_ADD, 4, UI_Color_Cyan, 3, 240, 770, 330, 735);
	
	//���Ͻ� CV����״̬
	Rectangle_Draw(&gCVfb_sts_box, "989", UI_Graph_ADD, 4, UI_Color_White, 3, TopLeft_CV_FEEDBACK_STATUS_on_OFF_START_X, TopLeft_CV_FEEDBACK_STATUS_on_OFF_START_Y, TopLeft_CV_FEEDBACK_STATUS_on_OFF_END_X, TopLeft_CV_FEEDBACK_STATUS_on_OFF_END_Y);
	
	//CV���� �Ƿ���׼��Ŀ�� Ȧ
	Circle_Draw(&gEnemyDetected_circle, "990", UI_Graph_ADD, 4, UI_Color_Cyan, 15, TopLeft_Cir_on_cv_DET_START_X, TopLeft_Cir_on_cv_DET_START_Y, TopLeft_Cir_on_cv_DET_radius);
	
	Float_Draw(&fProjSLim, "992", UI_Graph_ADD, 4, UI_Color_Main, 20, 2, 3, 240, 720, 15.00);
	Float_Draw(&fDis, "991", UI_Graph_ADD, 4, UI_Color_Main, 20, 2, 3, 240, 680, 1.00);
	
	UI_ReFresh(2, fCapVolt, fCapPct);
	UI_ReFresh(2, fProjSLim, fDis);
	UI_ReFresh(2, gEnemyDetected_circle, gCVfb_sts_box);
	//UI_ReFresh(5, fCapVolt, fCapPct, fProjSLim, fDis, gEnemyDetected_circle);
	UI_ReFresh(5, gChassisSts_box, gSPINSts_box, gCVSts_box, gGunSts_box, gABoxSts_box);
    //UI ��ʼ���� + ���ͽ���
	
	//��������
//		miniPC_info.enemy_detected = 1;
//		miniPC_info.cv_status = 1;
//		miniPC_info.dis_raw = 0x00;

	/*	��װ�װ�� 230mm 
	*/
	while (1)
    {
		//�Ȼ���̬ͼ��; ռ��ͼ�� 0, 1, 2, 3
		//���Ͻ�
		//Cap Pct:���� ��װ�ֺ�20 �� ͼ���߿� 3
		Char_Draw(&strCapPct, "008", UI_Graph_ADD, 2, UI_Color_Yellow, 20, 17, 3, CAP_PCT_X, CAP_PCT_Y,   "Cap Pct:        %");
	    //Cap Volt:���� ��װ�ֺ�20 �� ͼ���߿� 3
		Char_Draw(&strCapVolt, "009", UI_Graph_ADD, 2, UI_Color_Yellow, 20, 9, 3, CAP_VOLT_X, CAP_VOLT_Y, "Cap Volt:");  

		//����״̬		
		//Char_Draw(&strChassisSts, "010", UI_Graph_ADD, 2, UI_Color_Yellow, 20, 10, 3, CHASSIS_STS_X, CHASSIS_STS_Y, "NORM BOOST");
		//Char_Draw(&strSPINSts, "011", UI_Graph_ADD, 2, UI_Color_Yellow, 20, 9, 3, SPIN_STS_X, SPIN_STS_Y,           "FOLL SPIN");
		//New position ����״̬
		Char_Draw(&strChassisSts, "010", UI_Graph_ADD, 2, UI_Color_Yellow, 20, 10, 3, CHASSIS_STS_X-80, CHASSIS_STS_Y-95, "NORM BOOST");
		Char_Draw(&strSPINSts, "011", UI_Graph_ADD, 2, UI_Color_Yellow, 20, 9, 3, SPIN_STS_X-80, SPIN_STS_Y-95,           "FOLL SPIN");

		//�м�
		//Aim ����
		Line_Draw(&gAimVertL, "001", UI_Graph_ADD, 2, UI_Color_Orange, 1, 960-9, 540, 960-9, 0);
			
		//2m
		//Line_Draw(&gAimHorizL2m, "003", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960-50), (540-24), (960+50), (540-24));//big armor plate
		Line_Draw(&gAimHorizL2m, "003", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960-30-9), (540-24), (960+30-9), (540-24)); // small armor plate
		//4m
		//Line_Draw(&gAimHorizL4m, "004", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960-25), (540-45), (960+25), (540-45));
		Line_Draw(&gAimHorizL4m, "004", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960-15-9), (540-45), (960+15-9), (540-45));		
		//5 (540-150) "010"
	//	Line_Draw(&gAimHorizL5m, "005", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960-21), (540-66), (960+21), (540-66));
		Line_Draw(&gAimHorizL5m, "005", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960-12-9), (540-66), (960+12-9), (540-66));
		//7 (540-170) "011"
		//Line_Draw(&gAimHorizL7m, "006", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960-15), (540-95), (960+15), (540-95));
		Line_Draw(&gAimHorizL7m, "006", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960-9-9), (540-95), (960+9-9), (540-95));		
		//8 (540-190) "012"
		//Line_Draw(&gAimHorizL8m, "007", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960-12), (540-113), (960+12), (540-113));
		Line_Draw(&gAimHorizL8m, "007", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960-7-9), (540-113), (960+7-9), (540-113));
		// left side line drawing
		Line_Draw(&left8to7, "017", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960-12-9), (540-113), (960-15-9),(540-95) );
		Line_Draw(&left7to5, "018", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960-15-9), (540-95), (960-21-9), (540-66));
		Line_Draw(&left5to4, "019", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960-21-9), (540-66), (960-25-9), (540-45));
		Line_Draw(&left4to2, "020", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960-25-9), (540-45), (960-50-9), (540-24));
		// right side line drawing
		Line_Draw(&right8to7, "021", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960+12-9), (540-113), (960+15-9),(540-95) );
		Line_Draw(&right7to5, "022", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960+15-9), (540-95), (960+21-9), (540-66));
		Line_Draw(&right5to4, "023", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960+21-9), (540-66), (960+25-9), (540-45));
		Line_Draw(&right4to2, "024", UI_Graph_ADD, 2, UI_Color_Yellow, 1, (960+25-9), (540-45), (960+50-9), (540-24));


		//��߻�������Ϣ
		Char_Draw(&strCVSts, "012", UI_Graph_ADD, 2, UI_Color_Yellow, 20, 19, 3, CV_STS_X, CV_STS_Y,                              			   "CV:  OFF  AID  LOCK");  
		Char_Draw(&strGunSts, "013", UI_Graph_ADD, 2, UI_Color_Yellow, 20, 19, 3, GUN_STS_X, GUN_STS_Y,                           			   "GUN: OFF  SEMI AUTO");  
		Char_Draw(&strABoxSts, "014", UI_Graph_ADD, 2, UI_Color_Yellow, 20, 14, 3, AmmoBox_cover_STS_X, AmmoBox_cover_STS_Y,      			   "ABC: OFF  OPEN");
		Char_Draw(&strProjSLimSts, "015", UI_Graph_ADD, 2, UI_Color_Yellow, 20, 8, 3, Enemy_dis_STS_X, Enemy_dis_STS_Y, 									 "DS:    m");
		Char_Draw(&strDisSts, "016", UI_Graph_ADD, 2, UI_Color_Yellow, 20, 10, 3, Projectile_speed_lim_STS_X, Projectile_speed_lim_STS_Y,  "PL:    m/s");
		

		//���� ̬ͬͼ�����������------------
		ui_coord_update();
		//��̬ͼ�� ռ��ͼ�� 4,5,6,7 ****�������/���˶���֮��ǵ�Ҫ��ui_dynamic_crt_send_fuc() ��Ҳ��/��****
		//���Ͻ� �������ݵ�ѹ
		//����ʱsuperCap_info.VBKelvin_fromCap�޸�Ϊ 0
	    Float_Draw(&fCapVolt, "999", UI_Graph_Change, 4, UI_Color_Main, 20, 2, 3, 1620, 810, ui_info.cap_volt);
		//new position �м�
		//Float_Draw(&fCapVolt, "999", UI_Graph_Change, 4, UI_Color_Main, 20, 2, 3, 960-20, 200, ui_info.cap_volt);
		//���Ͻ� �������ݰٷֱ�
		//����ʱsuperCap_info.EBPct_fromCap�޸�Ϊ 0
		Float_Draw(&fCapPct, "998", UI_Graph_Change, 4, UI_Color_Main, 20, 2, 3, 1620, 840, ui_info.cap_pct);
		//new position with number size = 40
		//Float_Draw(&fCapPct, "998", UI_Graph_Change, 4, UI_Color_Main, 40, 2, 3, 960-20, 250, ui_info.cap_pct);
		//new position of rectangle ���ϽǷ���
		Rectangle_Draw(&gChassisSts_box, "997", UI_Graph_Change, 4, UI_Color_Cyan, 3, ui_info.box_chassis_sts_coord[0]-80, ui_info.box_chassis_sts_coord[1]-95, ui_info.box_chassis_sts_coord[2]-80, ui_info.box_chassis_sts_coord[3]-95);
		Rectangle_Draw(&gSPINSts_box, "996", UI_Graph_Change, 4, UI_Color_Cyan, 3, ui_info.box_spin_sts_coord[0]-80, ui_info.box_spin_sts_coord[1]-95, ui_info.box_spin_sts_coord[2]-80, ui_info.box_spin_sts_coord[3]-95);
		
		//���Ͻ�---��δ��
		Rectangle_Draw(&gCVSts_box, "995", UI_Graph_Change, 4, UI_Color_Cyan, 3, ui_info.box_cv_sts_coord[0], ui_info.box_cv_sts_coord[1], ui_info.box_cv_sts_coord[2], ui_info.box_cv_sts_coord[3]);
		Rectangle_Draw(&gGunSts_box, "994", UI_Graph_Change, 4, UI_Color_Cyan, 3, ui_info.box_gun_sts_coord[0], ui_info.box_gun_sts_coord[1], ui_info.box_gun_sts_coord[2], ui_info.box_gun_sts_coord[3]);
		Rectangle_Draw(&gABoxSts_box, "993", UI_Graph_Change, 4, UI_Color_Cyan, 3, ui_info.box_ammoBox_sts_coord[0], ui_info.box_ammoBox_sts_coord[1], ui_info.box_ammoBox_sts_coord[2], ui_info.box_ammoBox_sts_coord[3]);
		
		
		Float_Draw(&fProjSLim, "992", UI_Graph_Change, 4, UI_Color_Main, 20, 2, 3, 240, 720, ui_info.enemy_dis);
		Float_Draw(&fDis, "991", UI_Graph_Change, 4, UI_Color_Main, 20, 2, 3, 240, 680, ui_info.proj_speed_limit);
		
		//CV�Ƿ�ʶ��Ŀ��
		if(miniPC_info.enemy_detected == 1)
		{
			Circle_Draw(&gEnemyDetected_circle, "990", UI_Graph_Change, 4, UI_Color_Green, 15, TopLeft_Cir_on_cv_DET_START_X, TopLeft_Cir_on_cv_DET_START_Y, TopLeft_Cir_on_cv_DET_radius);
		}
		else
		{
			Circle_Draw(&gEnemyDetected_circle, "990", UI_Graph_Change, 4, UI_Color_Cyan, 15, TopLeft_Cir_on_cv_DET_START_X, TopLeft_Cir_on_cv_DET_START_Y, TopLeft_Cir_on_cv_DET_radius);
		}
		
		Rectangle_Draw(&gCVfb_sts_box, "989", UI_Graph_Change, 4, UI_Color_White, 3, ui_info.box_cv_feedback_sts[0], ui_info.box_cv_feedback_sts[1], ui_info.box_cv_feedback_sts[2], ui_info.box_cv_feedback_sts[3]);
		
		//****������ ���� ��/���˶���֮��ǵ�Ҫ��ui_dynamic_crt_send_fuc() ��Ҳ��/��****
		
		//��ɻ��� ��ʼ���� �ȷ���̬
		//refresh UI and String(Char)
		UI_ReFresh(1,gAimVertL);
		UI_ReFresh(5, gAimHorizL2m, gAimHorizL4m, gAimHorizL5m, gAimHorizL7m, gAimHorizL8m);
		UI_ReFresh(5, left8to7, left7to5,left5to4,left4to2, right8to7);
		UI_ReFresh(5,right5to4,right4to2 );
		UI_ReFresh(5, right7to5);
		//Right
		//Char_ReFresh(strCapVolt);
		//Char_ReFresh(strCapPct);	
		Char_ReFresh(strChassisSts);
		Char_ReFresh(strSPINSts);
		//Left
		Char_ReFresh(strCVSts);
		Char_ReFresh(strGunSts);
		Char_ReFresh(strABoxSts);
		Char_ReFresh(strProjSLimSts);
		Char_ReFresh(strDisSts);
		
		//��̬���޸� ����
		UI_ReFresh(2, fCapVolt, fCapPct);
		UI_ReFresh(2, fProjSLim, fDis);
		UI_ReFresh(2, gEnemyDetected_circle, gCVfb_sts_box);
		//UI_ReFresh(5, fCapVolt, fCapPct, fProjSLim, fDis, gEnemyDetected_circle);
		UI_ReFresh(5, gChassisSts_box, gSPINSts_box, gCVSts_box, gGunSts_box, gABoxSts_box);
		
		//��ʱ����һ�ζ�̬��--------------
		if(xTaskGetTickCount() - ui_dynamic_crt_sendFreq > ui_dynamic_crt_send_TimeStamp)
		{
				ui_dynamic_crt_send_TimeStamp = xTaskGetTickCount(); //����ʱ��� 
				ui_dynamic_crt_send_fuc(); //��ʱ����, �ڿͻ���ADDһ�ζ�̬��ͼ��
		}
	 
		vTaskDelay(100);
		//
		client_ui_count_ref++;
		
		if(client_ui_count_ref % 10 == 0)
			client_ui_test_flag = -1 * client_ui_test_flag;
				
			
#if INCLUDE_uxTaskGetStackHighWaterMark
    client_ui_task_high_water = uxTaskGetStackHighWaterMark(NULL);
#endif	
	}
}

/*ͨ���ж���� ״̬�� -> ui���� */
void ui_coord_update()
{
	//���� turbo ģʽ���ж�
	if(turboMode == 0)
	{
	 ui_info.ui_chassis_sts = NORM;
	 ui_info.box_chassis_sts_coord[0] = TopRight_REC_on_NORM_START_X;
	 ui_info.box_chassis_sts_coord[1] = TopRight_REC_on_NORM_START_Y;
	 ui_info.box_chassis_sts_coord[2] = TopRight_REC_on_NORM_END_X;
	 ui_info.box_chassis_sts_coord[3] = TopRight_REC_on_NORM_END_Y;
	}
	else
	{
	 ui_info.ui_chassis_sts = BOOST;
	 ui_info.box_chassis_sts_coord[0] = TopRight_REC_on_BOOST_START_X;
	 ui_info.box_chassis_sts_coord[1] = TopRight_REC_on_BOOST_START_Y;
	 ui_info.box_chassis_sts_coord[2] = TopRight_REC_on_BOOST_END_X;
	 ui_info.box_chassis_sts_coord[3] = TopRight_REC_on_BOOST_END_Y;
	}

	/*
	swing_flag��С����״̬��
	swing_flag = 0 ��С����
	swing_flag = 1 ��С����
	*/
	if(swing_flag == 0)
	{
	 ui_info.ui_spin_sts = spinFOLL;
	 ui_info.box_spin_sts_coord[0] = TopRight_REC_on_FOLL_START_X;
	 ui_info.box_spin_sts_coord[1] = TopRight_REC_on_FOLL_START_Y;
	 ui_info.box_spin_sts_coord[2] = TopRight_REC_on_FOLL_END_X;
	 ui_info.box_spin_sts_coord[3] = TopRight_REC_on_FOLL_END_Y;
	}
	else
	{
	 ui_info.ui_spin_sts = spinSPIN;
	 ui_info.box_spin_sts_coord[0] = TopRight_REC_on_SPIN_START_X;
	 ui_info.box_spin_sts_coord[1] = TopRight_REC_on_SPIN_START_Y;
	 ui_info.box_spin_sts_coord[2] = TopRight_REC_on_SPIN_END_X;
	 ui_info.box_spin_sts_coord[3] = TopRight_REC_on_SPIN_END_Y;
	}

	//CV ״̬�� ״̬ //�Զ���׼����״̬ 0�� 1�Զ���׼
	if(miniPC_info.autoAimFlag == 0)
	{
	 ui_info.ui_cv_sts = cvOFF;
	 ui_info.box_cv_sts_coord[0] = TopLeft_REC_on_cv_OFF_START_X;
	 ui_info.box_cv_sts_coord[1] = TopLeft_REC_on_cv_OFF_START_Y;
	 ui_info.box_cv_sts_coord[2] = TopLeft_REC_on_cv_OFF_END_X;
	 ui_info.box_cv_sts_coord[3] = TopLeft_REC_on_cv_OFF_END_Y;
	}
	else if(miniPC_info.autoAimFlag == 1)
	{
	 ui_info.ui_cv_sts = cvAID;
	 ui_info.box_cv_sts_coord[0] = TopLeft_REC_on_cv_AID_START_X;
	 ui_info.box_cv_sts_coord[1] = TopLeft_REC_on_cv_AID_START_Y;
	 ui_info.box_cv_sts_coord[2] = TopLeft_REC_on_cv_AID_END_X;
	 ui_info.box_cv_sts_coord[3] = TopLeft_REC_on_cv_AID_END_Y;
	}
	else if(miniPC_info.autoAimFlag == 2)
	{
	 ui_info.ui_cv_sts = cvLOCK;
	 ui_info.box_cv_sts_coord[0] = TopLeft_REC_on_cv_LOCK_START_X;
	 ui_info.box_cv_sts_coord[1] = TopLeft_REC_on_cv_LOCK_START_Y;
	 ui_info.box_cv_sts_coord[2] = TopLeft_REC_on_cv_LOCK_END_X;
	 ui_info.box_cv_sts_coord[3] = TopLeft_REC_on_cv_LOCK_END_Y;
	}

	//CV feedback ״̬��
	if(miniPC_info.cv_status == 0)
	{
	 ui_info.ui_cv_feedback_sts = cvOFF;
	 ui_info.box_cv_feedback_sts[0] = TopLeft_CV_FEEDBACK_STATUS_on_OFF_START_X;
	 ui_info.box_cv_feedback_sts[1] = TopLeft_CV_FEEDBACK_STATUS_on_OFF_START_Y;
	 ui_info.box_cv_feedback_sts[2] = TopLeft_CV_FEEDBACK_STATUS_on_OFF_END_X;
	 ui_info.box_cv_feedback_sts[3] = TopLeft_CV_FEEDBACK_STATUS_on_OFF_END_Y;
	}
	else if(miniPC_info.cv_status == 1)
	{
	 ui_info.ui_cv_feedback_sts = cvAID;
	 ui_info.box_cv_feedback_sts[0] = TopLeft_CV_FEEDBACK_STATUS_on_AID_START_X;
	 ui_info.box_cv_feedback_sts[1] = TopLeft_CV_FEEDBACK_STATUS_on_AID_START_Y;
	 ui_info.box_cv_feedback_sts[2] = TopLeft_CV_FEEDBACK_STATUS_on_AID_END_X;
	 ui_info.box_cv_feedback_sts[3] = TopLeft_CV_FEEDBACK_STATUS_on_AID_END_Y;
	}
	else if(miniPC_info.cv_status == 2)
	{
	 ui_info.ui_cv_feedback_sts = cvLOCK;
	 ui_info.box_cv_feedback_sts[0] = TopLeft_CV_FEEDBACK_STATUS_on_LOCK_START_X;
	 ui_info.box_cv_feedback_sts[1] = TopLeft_CV_FEEDBACK_STATUS_on_LOCK_START_Y;
	 ui_info.box_cv_feedback_sts[2] = TopLeft_CV_FEEDBACK_STATUS_on_LOCK_END_X;
	 ui_info.box_cv_feedback_sts[3] = TopLeft_CV_FEEDBACK_STATUS_on_LOCK_END_Y;
	}

	//CV״̬��������ʾ
	if(toe_is_error(MINIPC_TOE))
	{
	 ui_info.ui_cv_feedback_sts = cvOFF;
	 ui_info.box_cv_feedback_sts[0] = TopLeft_CV_FEEDBACK_STATUS_on_OFF_START_X;
	 ui_info.box_cv_feedback_sts[1] = TopLeft_CV_FEEDBACK_STATUS_on_OFF_START_Y;
	 ui_info.box_cv_feedback_sts[2] = TopLeft_CV_FEEDBACK_STATUS_on_OFF_END_X;
	 ui_info.box_cv_feedback_sts[3] = TopLeft_CV_FEEDBACK_STATUS_on_OFF_END_Y;
	}

	//GUN ״̬�� ״̬
	if(shoot_control.shoot_mode == SHOOT_STOP)
	{
	 ui_info.ui_gun_sts = gunOFF;
	 ui_info.box_gun_sts_coord[0] = TopLeft_REC_on_gun_OFF_START_X;
	 ui_info.box_gun_sts_coord[1] = TopLeft_REC_on_gun_OFF_START_Y;
	 ui_info.box_gun_sts_coord[2] = TopLeft_REC_on_gun_OFF_END_X;
	 ui_info.box_gun_sts_coord[3] = TopLeft_REC_on_gun_OFF_END_Y;
	}
	else if(shoot_control.user_fire_ctrl == user_SHOOT_AUTO)
	{
	 ui_info.ui_gun_sts = gunAUTO;
	 ui_info.box_gun_sts_coord[0] = TopLeft_REC_on_gun_AUTO_START_X;
	 ui_info.box_gun_sts_coord[1] = TopLeft_REC_on_gun_AUTO_START_Y;
	 ui_info.box_gun_sts_coord[2] = TopLeft_REC_on_gun_AUTO_END_X;
	 ui_info.box_gun_sts_coord[3] = TopLeft_REC_on_gun_AUTO_END_Y;
	}
	else if(shoot_control.user_fire_ctrl == user_SHOOT_SEMI)
	{
	 ui_info.ui_gun_sts = gunSEMI;
	 ui_info.box_gun_sts_coord[0] = TopLeft_REC_on_gun_SEMI_START_X;
	 ui_info.box_gun_sts_coord[1] = TopLeft_REC_on_gun_SEMI_START_Y;
	 ui_info.box_gun_sts_coord[2] = TopLeft_REC_on_gun_SEMI_END_X;
	 ui_info.box_gun_sts_coord[3] = TopLeft_REC_on_gun_SEMI_END_Y;
	}

	//Ammo Box ״̬�� ״̬
	if(shoot_control.ammoBox_sts == ammoOFF)
	{
	 ui_info.ui_ammoBox_sts = ammoOFF;
	 //ui_info.box_ammoBox_sts_coord
	 ui_info.box_ammoBox_sts_coord[0] = TopLeft_REC_on_ammo_OFF_START_X;
	 ui_info.box_ammoBox_sts_coord[1] = TopLeft_REC_on_ammo_OFF_START_Y;
	 ui_info.box_ammoBox_sts_coord[2] = TopLeft_REC_on_ammo_OFF_END_X;
	 ui_info.box_ammoBox_sts_coord[3] = TopLeft_REC_on_ammo_OFF_END_Y;
	}
	else if(shoot_control.ammoBox_sts == ammoOPEN)
	{
	 ui_info.ui_ammoBox_sts = ammoOPEN;
	 //ui_info.box_ammoBox_sts_coord
	 ui_info.box_ammoBox_sts_coord[0] = TopLeft_REC_on_ammo_OPEN_START_X;
	 ui_info.box_ammoBox_sts_coord[1] = TopLeft_REC_on_ammo_OPEN_START_Y;
	 ui_info.box_ammoBox_sts_coord[2] = TopLeft_REC_on_ammo_OPEN_END_X;
	 ui_info.box_ammoBox_sts_coord[3] = TopLeft_REC_on_ammo_OPEN_END_Y;
	}

	//��ʼ��������صĶ��� ���弴�õĳ������ݿ��ư� �ж�
	if(current_superCap == SuperCap_ID)
	{
	 if(toe_is_error(SUPERCAP_TOE))
	 {
		 ui_info.cap_pct = 0.0f;
		 ui_info.cap_volt = 0.0f;
	 }
	 else
	 {
		 ui_info.cap_pct = superCap_info.EBPct_fromCap;
		 ui_info.cap_volt = superCap_info.VBKelvin_fromCap;
	 }
	}
	else
	{
	 if(toe_is_error(WULIE_CAP_TOE))
	 {
		 ui_info.cap_pct = 0.0f;
		 ui_info.cap_volt = 0.0f;
	 }
	 else
	 {
		 ui_info.cap_pct = wulie_Cap_info.EBPct;
	   ui_info.cap_volt = wulie_Cap_info.cap_voltage;
	 }
	}

	ui_info.enemy_dis = miniPC_info.dis;
	ui_info.proj_speed_limit = get_shooter_id1_17mm_speed_limit();
}

void ui_dynamic_crt_send_fuc()
{
	//���Ͻ� �������ݵ�ѹ
	//����ʱsuperCap_info.VBKelvin_fromCap�޸�Ϊ 0
	Float_Draw(&fCapVolt, "999", UI_Graph_ADD, 4, UI_Color_Main, 20, 2, 3, 1620, 810, ui_info.cap_volt);
	//new postiion of voltage of super-cap
	//Float_Draw(&fCapVolt, "999", UI_Graph_ADD, 4, UI_Color_Main, 20, 2, 3, 960-20, 200, ui_info.cap_volt);
	//���Ͻ� �������ݰٷֱ�
	//����ʱsuperCap_info.EBPct_fromCap�޸�Ϊ 0
	Float_Draw(&fCapPct, "998", UI_Graph_ADD, 4, UI_Color_Main, 20, 2, 3, 1620, 840, ui_info.cap_pct);
	//new position with larger size of number size = 40
	//Float_Draw(&fCapPct, "998", UI_Graph_ADD, 4, UI_Color_Main, 40, 2, 3, 960-20, 250, ui_info.cap_pct);
	//���ϽǷ���
	Rectangle_Draw(&gChassisSts_box, "997", UI_Graph_ADD, 4, UI_Color_Cyan, 3, ui_info.box_chassis_sts_coord[0], ui_info.box_chassis_sts_coord[1], ui_info.box_chassis_sts_coord[2], ui_info.box_chassis_sts_coord[3]);
	Rectangle_Draw(&gSPINSts_box, "996", UI_Graph_ADD, 4, UI_Color_Cyan, 3, ui_info.box_spin_sts_coord[0], ui_info.box_spin_sts_coord[1], ui_info.box_spin_sts_coord[2], ui_info.box_spin_sts_coord[3]);
				
	//���Ͻ�---��δ��
	Rectangle_Draw(&gCVSts_box, "995", UI_Graph_ADD, 4, UI_Color_Cyan, 3, ui_info.box_cv_sts_coord[0], ui_info.box_cv_sts_coord[1], ui_info.box_cv_sts_coord[2], ui_info.box_cv_sts_coord[3]);
	Rectangle_Draw(&gGunSts_box, "994", UI_Graph_ADD, 4, UI_Color_Cyan, 3, ui_info.box_gun_sts_coord[0], ui_info.box_gun_sts_coord[1], ui_info.box_gun_sts_coord[2], ui_info.box_gun_sts_coord[3]);
	Rectangle_Draw(&gABoxSts_box, "993", UI_Graph_ADD, 4, UI_Color_Cyan, 3, ui_info.box_ammoBox_sts_coord[0], ui_info.box_ammoBox_sts_coord[1], ui_info.box_ammoBox_sts_coord[2], ui_info.box_ammoBox_sts_coord[3]);
	
	Float_Draw(&fProjSLim, "992", UI_Graph_ADD, 4, UI_Color_Main, 20, 2, 3, 240, 720, ui_info.proj_speed_limit);
	Float_Draw(&fDis, "991", UI_Graph_ADD, 4, UI_Color_Main, 20, 2, 3, 240, 680, ui_info.enemy_dis);
	
	//CV�Ƿ�ʶ��Ŀ��
	if(miniPC_info.enemy_detected == 1)
	{
		Circle_Draw(&gEnemyDetected_circle, "990", UI_Graph_ADD, 4, UI_Color_Green, 15, TopLeft_Cir_on_cv_DET_START_X, TopLeft_Cir_on_cv_DET_START_Y, TopLeft_Cir_on_cv_DET_radius);
	}
	else
	{
		Circle_Draw(&gEnemyDetected_circle, "990", UI_Graph_ADD, 4, UI_Color_Cyan, 15, TopLeft_Cir_on_cv_DET_START_X, TopLeft_Cir_on_cv_DET_START_Y, TopLeft_Cir_on_cv_DET_radius);
	}
	
	Rectangle_Draw(&gCVfb_sts_box, "989", UI_Graph_ADD, 4, UI_Color_White, 3, ui_info.box_cv_feedback_sts[0], ui_info.box_cv_feedback_sts[1], ui_info.box_cv_feedback_sts[2], ui_info.box_cv_feedback_sts[3]);
				
	//�������ƽ���		
				
	//��̬���޸� ����
	UI_ReFresh(2, fCapVolt, fCapPct);
	UI_ReFresh(2, fProjSLim, fDis);
	UI_ReFresh(2, gEnemyDetected_circle, gCVfb_sts_box);
	UI_ReFresh(5, gChassisSts_box, gSPINSts_box, gCVSts_box, gGunSts_box, gABoxSts_box);
}

////�ȿ�ʼɾ�� ͼ��4 5 6 7
//				delLayer.Delete_Operate = UI_Data_Del_Layer;
//				delLayer.Layer = 4;
//				Delete_ReFresh(delLayer);
//			
//			  delLayer.Layer = 5;
//				Delete_ReFresh(delLayer);
//			
//			  delLayer.Layer = 6;
//				Delete_ReFresh(delLayer);
//			
//			  delLayer.Layer = 7;
//				Delete_ReFresh(delLayer);

/****************************************��������ӳ��************************************/
void UI_SendByte(unsigned char ch)
{	
	HAL_UART_Transmit(&huart6, (uint8_t*)&ch, 1,999);
}

/********************************************ɾ������*************************************
**������Del_Operate  ��Ӧͷ�ļ�ɾ������
        Del_Layer    Ҫɾ���Ĳ� ȡֵ0-9
*****************************************************************************************/

void UI_Delete(u8 Del_Operate,u8 Del_Layer)
{

	unsigned char *framepoint;                      //��дָ��
	u16 frametail=0xFFFF;                        //CRC16У��ֵ
	int loop_control;                       //For����ѭ������

	UI_Packhead framehead;
	UI_Data_Operate datahead;
	UI_Data_Delete del;

	framepoint=(unsigned char *)&framehead;

	framehead.SOF=UI_SOF;
	framehead.Data_Length=8;
	framehead.Seq=UI_Seq;
	framehead.CRC8=Get_CRC8_Check_Sum_UI(framepoint,4,0xFF);
	framehead.CMD_ID=UI_CMD_Robo_Exchange;                   //����ͷ����

	datahead.Data_ID=UI_Data_ID_Del;
	 /*SZL 6-16-2022 ��̬����ȡ ����ϵͳID*/
	datahead.Sender_ID=get_robot_id(); //Robot_ID;
	switch(get_robot_id()){
		case UI_Data_RobotID_RHero:
			datahead.Receiver_ID = UI_Data_CilentID_RHero;//ΪӢ�۲����ֿͻ���(��)
			break;
		case UI_Data_RobotID_REngineer:
			datahead.Receiver_ID = UI_Data_CilentID_REngineer;
			break;
		case UI_Data_RobotID_RStandard1:
			datahead.Receiver_ID = UI_Data_CilentID_RStandard1;
			break;
		case UI_Data_RobotID_RStandard2:
			datahead.Receiver_ID = UI_Data_CilentID_RStandard2;
			break;
		case UI_Data_RobotID_RStandard3:
			datahead.Receiver_ID = UI_Data_CilentID_RStandard3;
			break;
		case UI_Data_RobotID_RAerial:
			datahead.Receiver_ID = UI_Data_CilentID_RAerial;
			break;

		case UI_Data_RobotID_BHero:
			datahead.Receiver_ID = UI_Data_CilentID_BHero;
			break;
		case UI_Data_RobotID_BEngineer:
			datahead.Receiver_ID = UI_Data_CilentID_BEngineer;
			break;
		case UI_Data_RobotID_BStandard1:
			datahead.Receiver_ID = UI_Data_CilentID_BStandard1;
			break;
		case UI_Data_RobotID_BStandard2:
			datahead.Receiver_ID = UI_Data_CilentID_BStandard2;
			break;
		case UI_Data_RobotID_BStandard3:
			datahead.Receiver_ID = UI_Data_CilentID_BStandard3;
			break;
		case UI_Data_RobotID_BAerial:
			datahead.Receiver_ID = UI_Data_CilentID_BAerial;
			break;
		default :
			datahead.Receiver_ID = Cilent_ID; //Ĭ�� ����������һ����ȥ
			datahead.Sender_ID = Robot_ID;
			break;
	 }//����������
   
	del.Delete_Operate=Del_Operate;
	del.Layer=Del_Layer;                                     //������Ϣ

	frametail=Get_CRC16_Check_Sum_UI(framepoint,sizeof(framehead),frametail);
	framepoint=(unsigned char *)&datahead;
	frametail=Get_CRC16_Check_Sum_UI(framepoint,sizeof(datahead),frametail);
	framepoint=(unsigned char *)&del;
	frametail=Get_CRC16_Check_Sum_UI(framepoint,sizeof(del),frametail);  //CRC16У��ֵ����

	framepoint=(unsigned char *)&framehead;
	for(loop_control=0;loop_control<sizeof(framehead);loop_control++)
	{
	  UI_SendByte(*framepoint);
	  framepoint++;
	}
	framepoint=(unsigned char *)&datahead;
	for(loop_control=0;loop_control<sizeof(datahead);loop_control++)
	{
	  UI_SendByte(*framepoint);
	  framepoint++;
	}
	framepoint=(unsigned char *)&del;
	for(loop_control=0;loop_control<sizeof(del);loop_control++)
	{
	  UI_SendByte(*framepoint);
	  framepoint++;
	}                                                                 //��������֡
	framepoint=(unsigned char *)&frametail;
	for(loop_control=0;loop_control<sizeof(frametail);loop_control++)
	{
	  UI_SendByte(*framepoint);
	  framepoint++;                                                  //����CRC16У��ֵ
	}

	UI_Seq++;                                                         //�����+1
}
/************************************************����ֱ��*************************************************
**������*image Graph_Data���ͱ���ָ�룬���ڴ��ͼ������
        imagename[3]   ͼƬ���ƣ����ڱ�ʶ����
        Graph_Operate   ͼƬ��������ͷ�ļ�
        Graph_Layer    ͼ��0-9
        Graph_Color    ͼ����ɫ
        Graph_Width    ͼ���߿�
        Start_x��Start_x    ��ʼ����
        End_x��End_y   ��������
**********************************************************************************************************/
        
void Line_Draw(Graph_Data *image,char imagename[3],u32 Graph_Operate,u32 Graph_Layer,u32 Graph_Color,u32 Graph_Width,u32 Start_x,u32 Start_y,u32 End_x,u32 End_y)
{
   int i;
//   for(i=0;i<3&&imagename[i]!='\0';i++)
//      image->graphic_name[2-i]=imagename[i];
	
	 //SZL 6-8-2022�޸� ͼ��������д�������˳���
	 for(i=0;i<3&&imagename[i]!='\0';i++)
      image->graphic_name[i]=imagename[i];
	
   image->operate_tpye = Graph_Operate;
   image->layer = Graph_Layer;
   image->color = Graph_Color;
   image->width = Graph_Width;
   image->start_x = Start_x;
   image->start_y = Start_y;
   image->end_x = End_x;
   image->end_y = End_y;
}

/************************************************���ƾ���*************************************************
**������*image Graph_Data���ͱ���ָ�룬���ڴ��ͼ������
        imagename[3]   ͼƬ���ƣ����ڱ�ʶ����
        Graph_Operate   ͼƬ��������ͷ�ļ�
        Graph_Layer    ͼ��0-9
        Graph_Color    ͼ����ɫ
        Graph_Width    ͼ���߿�
        Start_x��Start_x    ��ʼ����
        End_x��End_y   �������꣨�Զ������꣩
**********************************************************************************************************/
        
void Rectangle_Draw(Graph_Data *image,char imagename[3],u32 Graph_Operate,u32 Graph_Layer,u32 Graph_Color,u32 Graph_Width,u32 Start_x,u32 Start_y,u32 End_x,u32 End_y)
{
   int i;
//   for(i=0;i<3&&imagename[i]!=\0;i++)
//      image->graphic_name[2-i]=imagename[i];
	
	  //SZL 6-8-2022�޸� ͼ��������д�������˳���
	 for(i=0;i<3&&imagename[i]!='\0';i++)
      image->graphic_name[i]=imagename[i];
	
   image->graphic_tpye = UI_Graph_Rectangle;
   image->operate_tpye = Graph_Operate;
   image->layer = Graph_Layer;
   image->color = Graph_Color;
   image->width = Graph_Width;
   image->start_x = Start_x;
   image->start_y = Start_y;
   image->end_x = End_x;
   image->end_y = End_y;
}

/************************************************������Բ*************************************************
**������*image Graph_Data���ͱ���ָ�룬���ڴ��ͼ������
        imagename[3]   ͼƬ���ƣ����ڱ�ʶ����
        Graph_Operate   ͼƬ��������ͷ�ļ�
        Graph_Layer    ͼ��0-9
        Graph_Color    ͼ����ɫ
        Graph_Width    ͼ���߿�
        Start_x��Start_x    Բ������
        Graph_Radius  ͼ�ΰ뾶
**********************************************************************************************************/
        
void Circle_Draw(Graph_Data *image,char imagename[3],u32 Graph_Operate,u32 Graph_Layer,u32 Graph_Color,u32 Graph_Width,u32 Start_x,u32 Start_y,u32 Graph_Radius)
{
   int i;
//   for(i=0;i<3&&imagename[i]!=\0;i++)
//      image->graphic_name[2-i]=imagename[i];
	
	  //SZL 6-8-2022�޸� ͼ��������д�������˳���
	 for(i=0;i<3&&imagename[i]!='\0';i++)
      image->graphic_name[i]=imagename[i];
	
   image->graphic_tpye = UI_Graph_Circle;
   image->operate_tpye = Graph_Operate;
   image->layer = Graph_Layer;
   image->color = Graph_Color;
   image->width = Graph_Width;
   image->start_x = Start_x;
   image->start_y = Start_y;
   image->radius = Graph_Radius;
}

/************************************************����Բ��*************************************************
**������*image Graph_Data���ͱ���ָ�룬���ڴ��ͼ������
        imagename[3]   ͼƬ���ƣ����ڱ�ʶ����
        Graph_Operate   ͼƬ��������ͷ�ļ�
        Graph_Layer    ͼ��0-9
        Graph_Color    ͼ����ɫ
        Graph_Width    ͼ���߿�
        Graph_StartAngle,Graph_EndAngle    ��ʼ����ֹ�Ƕ�
        Start_y,Start_y    Բ������
        x_Length,y_Length   x,y�������᳤���ο���Բ
**********************************************************************************************************/
        
void Arc_Draw(Graph_Data *image,char imagename[3],u32 Graph_Operate,u32 Graph_Layer,u32 Graph_Color,u32 Graph_StartAngle,u32 Graph_EndAngle,u32 Graph_Width,u32 Start_x,u32 Start_y,u32 x_Length,u32 y_Length)
{
   int i;
   
//   for(i=0;i<3&&imagename[i]!=\0;i++)
//      image->graphic_name[2-i]=imagename[i];
	
	  //SZL 6-8-2022�޸� ͼ��������д�������˳���
	 for(i=0;i<3&&imagename[i]!='\0';i++)
      image->graphic_name[i]=imagename[i];
	
   image->graphic_tpye = UI_Graph_Arc;
   image->operate_tpye = Graph_Operate;
   image->layer = Graph_Layer;
   image->color = Graph_Color;
   image->width = Graph_Width;
   image->start_x = Start_x;
   image->start_y = Start_y;
   image->start_angle = Graph_StartAngle;
   image->end_angle = Graph_EndAngle;
   image->end_x = x_Length;
   image->end_y = y_Length;
}



/************************************************���Ƹ���������*************************************************
**������*image Graph_Data���ͱ���ָ�룬���ڴ��ͼ������
        imagename[3]   ͼƬ���ƣ����ڱ�ʶ����
        Graph_Operate   ͼƬ��������ͷ�ļ�
        Graph_Layer    ͼ��0-9
        Graph_Color    ͼ����ɫ
        Graph_Width    ͼ���߿�
        Graph_Size     �ֺ�
        Graph_Digit    С��λ��
        Start_x��Start_x    ��ʼ����
        Graph_Float   Ҫ��ʾ�ı���
**********************************************************************************************************/
        
void Float_Draw(Float_Data *image,char imagename[3],u32 Graph_Operate,u32 Graph_Layer,u32 Graph_Color,u32 Graph_Size,u32 Graph_Digit,u32 Graph_Width,u32 Start_x,u32 Start_y,float Graph_Float)
{
   int i;
   
//   for(i=0;i<3&&imagename[i]!=\0;i++)
//      image->graphic_name[2-i]=imagename[i];
	
	  //SZL 6-8-2022�޸� ͼ��������д�������˳���
	 for(i=0;i<3&&imagename[i]!='\0';i++)
      image->graphic_name[i]=imagename[i];
	
   image->graphic_tpye = UI_Graph_Float;
   image->operate_tpye = Graph_Operate;
   image->layer = Graph_Layer;
   image->color = Graph_Color;
   image->width = Graph_Width;
   image->start_x = Start_x;
   image->start_y = Start_y;
   image->start_angle = Graph_Size;
   image->end_angle = Graph_Digit;
   image->graph_Float = (int32_t)(1000*Graph_Float);
	//SZL 6-8-2022 �޸�float����Э��
}



/************************************************�����ַ�������*************************************************
**������*image Graph_Data���ͱ���ָ�룬���ڴ��ͼ������
        imagename[3]   ͼƬ���ƣ����ڱ�ʶ����
        Graph_Operate   ͼƬ��������ͷ�ļ�
        Graph_Layer    ͼ��0-9
        Graph_Color    ͼ����ɫ
        Graph_Width    ͼ���߿�
        Graph_Size     �ֺ�
        Graph_Digit    �ַ�����
        Start_x��Start_x    ��ʼ����
        *Char_Data          �������ַ�����ʼ��ַ
**********************************************************************************************************/
        
void Char_Draw(String_Data *image,char imagename[3],u32 Graph_Operate,u32 Graph_Layer,u32 Graph_Color,u32 Graph_Size,u32 Graph_Digit,u32 Graph_Width,u32 Start_x,u32 Start_y,char *Char_Data)
{
   int i;
   
//   for(i=0;i<3&&imagename[i]!=\0;i++)
//      image->Graph_Control.graphic_name[2-i]=imagename[i];
	
	  //SZL 6-8-2022�޸� ͼ��������д�������˳���
	 for(i=0;i<3&&imagename[i]!='\0';i++)
      image->Graph_Control.graphic_name[i]=imagename[i];
	
   image->Graph_Control.graphic_tpye = UI_Graph_Char;
   image->Graph_Control.operate_tpye = Graph_Operate;
   image->Graph_Control.layer = Graph_Layer;
   image->Graph_Control.color = Graph_Color;
   image->Graph_Control.width = Graph_Width;
   image->Graph_Control.start_x = Start_x;
   image->Graph_Control.start_y = Start_y;
   image->Graph_Control.start_angle = Graph_Size;
   image->Graph_Control.end_angle = Graph_Digit;
   
   for(i=0;i<Graph_Digit;i++)
   {
      image->show_Data[i]=*Char_Data;
      Char_Data++;
   }
}

/************************************************UI���ͺ�����ʹ������Ч��*********************************
**������ cnt   ͼ�θ���
         ...   ͼ�α�������


Tips�����ú���ֻ������1��2��5��7��ͼ�Σ�������ĿЭ��δ�漰
**********************************************************************************************************/
int UI_ReFresh(int cnt,...)
{
   int i,n;
   Graph_Data imageData;
   unsigned char *framepoint;                      //��дָ��
   u16 frametail=0xFFFF;                        //CRC16У��ֵ
   
   UI_Packhead framehead;//frame_header(5-byte)+cmd_id(2-byte)
   UI_Data_Operate datahead;//�������ݰ�����һ��ͳһ�����ݶ�ͷ�ṹ: ����ID ������ �����ߵ�ID���������ݶ�
   
   va_list ap;
   va_start(ap,cnt);
   
   framepoint=(unsigned char *)&framehead;
   framehead.SOF=UI_SOF;
   framehead.Data_Length=6+cnt*15;
   framehead.Seq=UI_Seq;
   framehead.CRC8=Get_CRC8_Check_Sum_UI(framepoint,4,0xFF);
   framehead.CMD_ID=UI_CMD_Robo_Exchange;                   //����ͷ����
   
   switch(cnt)
   {
      case 1:
         datahead.Data_ID=UI_Data_ID_Draw1;
         break;
      case 2:
         datahead.Data_ID=UI_Data_ID_Draw2;
         break;
      case 5:
         datahead.Data_ID=UI_Data_ID_Draw5;
         break;
      case 7:
         datahead.Data_ID=UI_Data_ID_Draw7;
         break;
      default:
         return (-1);
   }
	 
    /*SZL 6-16-2022 ��̬����ȡ ����ϵͳID*/
    datahead.Sender_ID=get_robot_id(); //Robot_ID;
	switch(get_robot_id()){
		case UI_Data_RobotID_RHero:
			datahead.Receiver_ID = UI_Data_CilentID_RHero;//ΪӢ�۲����ֿͻ���(��)
			break;
		case UI_Data_RobotID_REngineer:
			datahead.Receiver_ID = UI_Data_CilentID_REngineer;
			break;
		case UI_Data_RobotID_RStandard1:
			datahead.Receiver_ID = UI_Data_CilentID_RStandard1;
			break;
		case UI_Data_RobotID_RStandard2:
			datahead.Receiver_ID = UI_Data_CilentID_RStandard2;
			break;
		case UI_Data_RobotID_RStandard3:
			datahead.Receiver_ID = UI_Data_CilentID_RStandard3;
			break;
		case UI_Data_RobotID_RAerial:
			datahead.Receiver_ID = UI_Data_CilentID_RAerial;
			break;

		case UI_Data_RobotID_BHero:
			datahead.Receiver_ID = UI_Data_CilentID_BHero;
			break;
		case UI_Data_RobotID_BEngineer:
			datahead.Receiver_ID = UI_Data_CilentID_BEngineer;
			break;
		case UI_Data_RobotID_BStandard1:
			datahead.Receiver_ID = UI_Data_CilentID_BStandard1;
			break;
		case UI_Data_RobotID_BStandard2:
			datahead.Receiver_ID = UI_Data_CilentID_BStandard2;
			break;
		case UI_Data_RobotID_BStandard3:
			datahead.Receiver_ID = UI_Data_CilentID_BStandard3;
			break;
		case UI_Data_RobotID_BAerial:
			datahead.Receiver_ID = UI_Data_CilentID_BAerial;
			break;
		default :
			datahead.Receiver_ID = Cilent_ID; //Ĭ�� ����������һ����ȥ
			datahead.Sender_ID = Robot_ID;
			break;
	}//����������
   
   framepoint=(unsigned char *)&framehead;
   frametail=Get_CRC16_Check_Sum_UI(framepoint,sizeof(framehead),frametail);
   framepoint=(unsigned char *)&datahead;
   frametail=Get_CRC16_Check_Sum_UI(framepoint,sizeof(datahead),frametail);          //CRC16У��ֵ���㣨���֣�
   
   framepoint=(unsigned char *)&framehead;
   for(i=0;i<sizeof(framehead);i++)
   {
      UI_SendByte(*framepoint);
      framepoint++;
   }
   framepoint=(unsigned char *)&datahead;
   for(i=0;i<sizeof(datahead);i++)
   {
      UI_SendByte(*framepoint);
      framepoint++;
   }
   
   for(i=0;i<cnt;i++)
   {
      imageData=va_arg(ap,Graph_Data);
      
      framepoint=(unsigned char *)&imageData;
      frametail=Get_CRC16_Check_Sum_UI(framepoint,sizeof(imageData),frametail);             //CRC16У��
      
      for(n=0;n<sizeof(imageData);n++)
      {
         UI_SendByte(*framepoint);
         framepoint++;             
      } //����ͼƬ֡
   }
   framepoint=(unsigned char *)&frametail;
   for(i=0;i<sizeof(frametail);i++)
   {
      UI_SendByte(*framepoint);
      framepoint++;                                                  //����CRC16У��ֵ
   }
   
   va_end(ap);
   
   UI_Seq++;                                                         //�����+1
   return 0;
}


/************************************************UI�����ַ���ʹ������Ч��*********************************
**������ cnt   ͼ�θ���
         ...   ͼ�α�������


Tips�����ú���ֻ������1��2��5��7��ͼ�Σ�������ĿЭ��δ�漰
**********************************************************************************************************/
int Char_ReFresh(String_Data string_Data)
{
   int i;
   String_Data imageData;
   unsigned char *framepoint;                      //��дָ��
   u16 frametail=0xFFFF;                        //CRC16У��ֵ
   
   UI_Packhead framehead;//frame_header(5-byte)+cmd_id(2-byte)
   UI_Data_Operate datahead;//�������ݰ�����һ��ͳһ�����ݶ�ͷ�ṹ: ����ID ������ �����ߵ�ID���������ݶ�
   imageData=string_Data;
   
   
   framepoint=(unsigned char *)&framehead;
   framehead.SOF=UI_SOF;
   framehead.Data_Length=6+45;
   framehead.Seq=UI_Seq;
   framehead.CRC8=Get_CRC8_Check_Sum_UI(framepoint,4,0xFF);
   framehead.CMD_ID=UI_CMD_Robo_Exchange;                   //����ͷ����
   

   datahead.Data_ID=UI_Data_ID_DrawChar;//SZL 6-8-2022 �޸�

    /*SZL 6-16-2022 ��̬����ȡ ����ϵͳID*/
    datahead.Sender_ID=get_robot_id(); //Robot_ID;
	switch(get_robot_id()){
		case UI_Data_RobotID_RHero:
			datahead.Receiver_ID = UI_Data_CilentID_RHero;//ΪӢ�۲����ֿͻ���(��)
			break;
		case UI_Data_RobotID_REngineer:
			datahead.Receiver_ID = UI_Data_CilentID_REngineer;
			break;
		case UI_Data_RobotID_RStandard1:
			datahead.Receiver_ID = UI_Data_CilentID_RStandard1;
			break;
		case UI_Data_RobotID_RStandard2:
			datahead.Receiver_ID = UI_Data_CilentID_RStandard2;
			break;
		case UI_Data_RobotID_RStandard3:
			datahead.Receiver_ID = UI_Data_CilentID_RStandard3;
			break;
		case UI_Data_RobotID_RAerial:
			datahead.Receiver_ID = UI_Data_CilentID_RAerial;
			break;

		case UI_Data_RobotID_BHero:
			datahead.Receiver_ID = UI_Data_CilentID_BHero;
			break;
		case UI_Data_RobotID_BEngineer:
			datahead.Receiver_ID = UI_Data_CilentID_BEngineer;
			break;
		case UI_Data_RobotID_BStandard1:
			datahead.Receiver_ID = UI_Data_CilentID_BStandard1;
			break;
		case UI_Data_RobotID_BStandard2:
			datahead.Receiver_ID = UI_Data_CilentID_BStandard2;
			break;
		case UI_Data_RobotID_BStandard3:
			datahead.Receiver_ID = UI_Data_CilentID_BStandard3;
			break;
		case UI_Data_RobotID_BAerial:
			datahead.Receiver_ID = UI_Data_CilentID_BAerial;
			break;
		default :
			datahead.Receiver_ID = Cilent_ID; //Ĭ�� ����������һ����ȥ
			datahead.Sender_ID = Robot_ID;
			break;
	 }//����������
   
   framepoint=(unsigned char *)&framehead;
   frametail=Get_CRC16_Check_Sum_UI(framepoint,sizeof(framehead),frametail);
   framepoint=(unsigned char *)&datahead;
   frametail=Get_CRC16_Check_Sum_UI(framepoint,sizeof(datahead),frametail);
   framepoint=(unsigned char *)&imageData;
   frametail=Get_CRC16_Check_Sum_UI(framepoint,sizeof(imageData),frametail);             //CRC16У��   //CRC16У��ֵ���㣨���֣�
   
   framepoint=(unsigned char *)&framehead;
   for(i=0;i<sizeof(framehead);i++)
   {
      UI_SendByte(*framepoint);
      framepoint++;
   }
   framepoint=(unsigned char *)&datahead;
   for(i=0;i<sizeof(datahead);i++)
   {
      UI_SendByte(*framepoint);
      framepoint++;
   }                                                   //���Ͳ�������  
   framepoint=(unsigned char *)&imageData;
   for(i=0;i<sizeof(imageData);i++)
   {
      UI_SendByte(*framepoint);
      framepoint++;             
   }                                               //����ͼƬ֡
   
   
   
   framepoint=(unsigned char *)&frametail;
   for(i=0;i<sizeof(frametail);i++)
   {
      UI_SendByte(*framepoint);
      framepoint++;                                                  //����CRC16У��ֵ
   }
   
   
   UI_Seq++;                                                         //�����+1
   return 0;
}

/*

*/
int Delete_ReFresh(UI_Data_Delete delete_Data)
{
	 int i;
   UI_Data_Delete imageData;
   unsigned char *framepoint;                      //��дָ��
   u16 frametail=0xFFFF;                        //CRC16У��ֵ
   
   UI_Packhead framehead;//frame_header(5-byte)+cmd_id(2-byte)
   UI_Data_Operate datahead;//�������ݰ�����һ��ͳһ�����ݶ�ͷ�ṹ: ����ID ������ �����ߵ�ID���������ݶ�
   imageData=delete_Data;
   
   
   framepoint=(unsigned char *)&framehead;
   framehead.SOF=UI_SOF;
   framehead.Data_Length=6+2;
   framehead.Seq=UI_Seq;
   framehead.CRC8=Get_CRC8_Check_Sum_UI(framepoint,4,0xFF);
   framehead.CMD_ID=UI_CMD_Robo_Exchange;                   //����ͷ����
   

   datahead.Data_ID=UI_Data_ID_Del;

    /*SZL 6-16-2022 ��̬����ȡ ����ϵͳID*/
    datahead.Sender_ID=get_robot_id(); //Robot_ID;
	switch(get_robot_id()){
		case UI_Data_RobotID_RHero:
			datahead.Receiver_ID = UI_Data_CilentID_RHero;//ΪӢ�۲����ֿͻ���(��)
			break;
		case UI_Data_RobotID_REngineer:
			datahead.Receiver_ID = UI_Data_CilentID_REngineer;
			break;
		case UI_Data_RobotID_RStandard1:
			datahead.Receiver_ID = UI_Data_CilentID_RStandard1;
			break;
		case UI_Data_RobotID_RStandard2:
			datahead.Receiver_ID = UI_Data_CilentID_RStandard2;
			break;
		case UI_Data_RobotID_RStandard3:
			datahead.Receiver_ID = UI_Data_CilentID_RStandard3;
			break;
		case UI_Data_RobotID_RAerial:
			datahead.Receiver_ID = UI_Data_CilentID_RAerial;
			break;

		case UI_Data_RobotID_BHero:
			datahead.Receiver_ID = UI_Data_CilentID_BHero;
			break;
		case UI_Data_RobotID_BEngineer:
			datahead.Receiver_ID = UI_Data_CilentID_BEngineer;
			break;
		case UI_Data_RobotID_BStandard1:
			datahead.Receiver_ID = UI_Data_CilentID_BStandard1;
			break;
		case UI_Data_RobotID_BStandard2:
			datahead.Receiver_ID = UI_Data_CilentID_BStandard2;
			break;
		case UI_Data_RobotID_BStandard3:
			datahead.Receiver_ID = UI_Data_CilentID_BStandard3;
			break;
		case UI_Data_RobotID_BAerial:
			datahead.Receiver_ID = UI_Data_CilentID_BAerial;
			break;
		default :
			datahead.Receiver_ID = Cilent_ID; //Ĭ�� ����������һ����ȥ
			datahead.Sender_ID = Robot_ID;
			break;
	 }//����������
   
   framepoint=(unsigned char *)&framehead;
   frametail=Get_CRC16_Check_Sum_UI(framepoint,sizeof(framehead),frametail);
   framepoint=(unsigned char *)&datahead;
   frametail=Get_CRC16_Check_Sum_UI(framepoint,sizeof(datahead),frametail);
   framepoint=(unsigned char *)&imageData;
   frametail=Get_CRC16_Check_Sum_UI(framepoint,sizeof(imageData),frametail);             //CRC16У��   //CRC16У��ֵ���㣨���֣�
   
   framepoint=(unsigned char *)&framehead;
   for(i=0;i<sizeof(framehead);i++)
   {
      UI_SendByte(*framepoint);
      framepoint++;
   }
   framepoint=(unsigned char *)&datahead;
   for(i=0;i<sizeof(datahead);i++)
   {
      UI_SendByte(*framepoint);
      framepoint++;
   }                                                   //���Ͳ�������  
   framepoint=(unsigned char *)&imageData;
   for(i=0;i<sizeof(imageData);i++)
   {
      UI_SendByte(*framepoint);
      framepoint++;             
   }                                               //����ͼƬ֡
   

   framepoint=(unsigned char *)&frametail;
   for(i=0;i<sizeof(frametail);i++)
   {
      UI_SendByte(*framepoint);
      framepoint++;                                                  //����CRC16У��ֵ
   }
   
   
   UI_Seq++;                                                         //�����+1
   return 0;
}

/*****************************************************CRC8У��ֵ����**********************************************/
const unsigned char CRC8_INIT_UI = 0xff; 
const unsigned char CRC8_TAB_UI[256] = 
{ 
	0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83, 0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41, 
	0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e, 0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc, 
	0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0, 0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62, 
	0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d, 0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff, 
	0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5, 0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07, 
	0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58, 0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a, 
	0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6, 0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24, 
	0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b, 0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9, 
	0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f, 0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd, 
	0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92, 0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50, 
	0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c, 0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee, 
	0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1, 0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73, 
	0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49, 0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b, 
	0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4, 0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16, 
	0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a, 0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8, 
	0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7, 0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35, 
};
unsigned char Get_CRC8_Check_Sum_UI(unsigned char *pchMessage,unsigned int dwLength,unsigned char ucCRC8) 
{ 
	unsigned char ucIndex; 
	while (dwLength--) 
	{ 
		ucIndex = ucCRC8^(*pchMessage++); 
		ucCRC8 = CRC8_TAB_UI[ucIndex]; 
	} 
	return(ucCRC8); 
}

uint16_t CRC_INIT_UI = 0xffff; 
const uint16_t wCRC_Table_UI[256] = 
{ 
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 
	0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7, 
	0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 
	0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876, 
	0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 
	0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5, 
	0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c, 
	0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 
	0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb, 
	0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3, 
	0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
	0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72, 
	0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 
	0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1, 
	0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 
	0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
	0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 
	0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff, 
	0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 
	0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 
	0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5, 
	0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 
	0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 
	0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c, 
	0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 
	0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb, 
	0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 
	0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a, 
	0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 
	0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9, 
	0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 
	0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};
/* 
** Descriptions: CRC16 checksum function 
** Input: Data to check,Stream length, initialized checksum 
** Output: CRC checksum 
*/ 
uint16_t Get_CRC16_Check_Sum_UI(uint8_t *pchMessage,uint32_t dwLength,uint16_t wCRC) 
{ 
	Uint8_t chData; 
	if (pchMessage == NULL) 
	{ 
		return 0xFFFF; 
	} 
	while(dwLength--) 
	{ 
		chData = *pchMessage++;
		(wCRC) = ((uint16_t)(wCRC) >> 8) ^ wCRC_Table_UI[((uint16_t)(wCRC) ^ (uint16_t)(chData)) & 
		0x00ff]; 
	} 
	return wCRC; 
}



