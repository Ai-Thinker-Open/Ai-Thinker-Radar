#ifndef ALGORITHM_CONFIG_H
#define ALGORITHM_CONFIG_H

/*	========================== 雷达参数 =============================	*/
#define PARAM_RANGE_RES                 75            //距离分辨率：75厘米

/*	=================================================================	*/


/*  ========================== 算法参数 =============================	*/
#define PARAM_MAX_MOTION_RANGE          600
#define PARAM_MAX_MOTIONLESS_RANGE      600
#define PARAM_MOTION_MAX                (PARAM_MAX_MOTION_RANGE/PARAM_RANGE_RES)
#define PARAM_MOTIONLESS_MAX            (PARAM_MAX_MOTIONLESS_RANGE/PARAM_RANGE_RES)


/*裸板*/
#define PARAM_MOTION_SENSITIBITY_RANG0  (50)
#define PARAM_MOTION_SENSITIBITY_RANG1  (50)
#define PARAM_MOTION_SENSITIBITY_RANG2  (40)
#define PARAM_MOTION_SENSITIBITY_RANG3  (40)
#define PARAM_MOTION_SENSITIBITY_RANG4  (40)
#define PARAM_MOTION_SENSITIBITY_RANG5  (40)
#define PARAM_MOTION_SENSITIBITY_RANG6  (30)
#define PARAM_MOTION_SENSITIBITY_RANG7  (30)
#define PARAM_MOTION_SENSITIBITY_RANG8  (30)


#define PARAM_MOTIONLESS_SENSITIBITY_RANG0    (5500000)//不参与判断
#define PARAM_MOTIONLESS_SENSITIBITY_RANG1    (5500000)//不参与判断
#define PARAM_MOTIONLESS_SENSITIBITY_RANG2 (40)
#define PARAM_MOTIONLESS_SENSITIBITY_RANG3 (40)
#define PARAM_MOTIONLESS_SENSITIBITY_RANG4 (40)
#define PARAM_MOTIONLESS_SENSITIBITY_RANG5 (40)
#define PARAM_MOTIONLESS_SENSITIBITY_RANG6 (15)
#define PARAM_MOTIONLESS_SENSITIBITY_RANG7 (15)
#define PARAM_MOTIONLESS_SENSITIBITY_RANG8 (15)

#define PARAM_OFF_TIME                  5       // s


/*	=================================================================	*/

#endif
