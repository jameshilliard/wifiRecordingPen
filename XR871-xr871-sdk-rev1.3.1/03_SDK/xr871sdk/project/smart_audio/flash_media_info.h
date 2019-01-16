#ifndef __FLASH_MEDIA_INFO_H_
#define __FLASH_MEDIA_INFO_H_


#define IS_ANYKA_AFRESH_NET_CONFIG_LENGTH    15444
#define IS_ANYKA_CONNECTED_SUCCESS_LENGTH    11664
#define IS_ANYKA_PLEASE_CONFIG_NET_LENGTH     9828
#define IS_ANYKA_RECOVER_DEVICE_LENGTH       57730
#define IS_BEGINSTUDYMODE_LENGTH              6156
#define IS_CLOSELEGWARN_LENGTH               16632
#define IS_RSET45_LENGTH                     20628
#define IS_SIT1_LENGTH                        3672
#define IS_SIT2_LENGTH                        4428
#define IS_SIT3_LENGTH                        3996
#define IS_SIT5_LENGTH                       10368
#define MF_TEST110_LENGTH                     4644
#define MF_TEST111_LENGTH                    13932
#define MF_TEST11_LENGTH                      4968
#define MF_TEST12_LENGTH                      8532
#define MF_TEST13_LENGTH                      9396
#define MF_TEST14_LENGTH                      7884
#define MF_TEST15_LENGTH                      7452
#define MF_TEST16_LENGTH                      4968
#define MF_TEST17_LENGTH                      5508
#define MF_TEST18_LENGTH                      7452
#define MF_TEST19_LENGTH                      5184

#define V00_IS_SENDFINISH_LENGTH              7339
#define V00_IS_SENDSTART_LENGTH               2917



#define IS_ANYKA_AFRESH_NET_CONFIG_FLASHADDR (0x40                                  +0x200000)
#define IS_ANYKA_CONNECTED_SUCCESS_FLASHADDR (IS_ANYKA_AFRESH_NET_CONFIG_FLASHADDR + 15444)
#define IS_ANYKA_PLEASE_CONFIG_NET_FLASHADDR (IS_ANYKA_CONNECTED_SUCCESS_FLASHADDR + 11664)
#define IS_ANYKA_RECOVER_DEVICE_FLASHADDR    (IS_ANYKA_PLEASE_CONFIG_NET_FLASHADDR +  9828)
#define IS_BEGINSTUDYMODE_FLASHADDR          (IS_ANYKA_RECOVER_DEVICE_FLASHADDR    + 57730)
#define IS_CLOSELEGWARN_FLASHADDR            (IS_BEGINSTUDYMODE_FLASHADDR          +  6156)
#define IS_RSET45_FLASHADDR                  (IS_CLOSELEGWARN_FLASHADDR            + 16632)
#define IS_SIT1_FLASHADDR                    (IS_RSET45_FLASHADDR                  + 20628)
#define IS_SIT2_FLASHADDR                    (IS_SIT1_FLASHADDR                    +  3672)
#define IS_SIT3_FLASHADDR                    (IS_SIT2_FLASHADDR                    +  4428)
#define IS_SIT5_FLASHADDR                    (IS_SIT3_FLASHADDR                    +  3996)
#define MF_TEST110_FLASHADDR                 (IS_SIT5_FLASHADDR                    + 10368)
#define MF_TEST111_FLASHADDR                 (MF_TEST110_FLASHADDR                 +  4644)
#define MF_TEST11_FLASHADDR                  (MF_TEST111_FLASHADDR                 + 13932)
#define MF_TEST12_FLASHADDR                  (MF_TEST11_FLASHADDR                  +  4968)
#define MF_TEST13_FLASHADDR                  (MF_TEST12_FLASHADDR                  +  8532)
#define MF_TEST14_FLASHADDR                  (MF_TEST13_FLASHADDR                  +  9396)
#define MF_TEST15_FLASHADDR                  (MF_TEST14_FLASHADDR                  +  7884)
#define MF_TEST16_FLASHADDR                  (MF_TEST15_FLASHADDR                  +  7452)
#define MF_TEST17_FLASHADDR                  (MF_TEST16_FLASHADDR                  +  4968)
#define MF_TEST18_FLASHADDR                  (MF_TEST17_FLASHADDR                  +  5508)
#define MF_TEST19_FLASHADDR                  (MF_TEST18_FLASHADDR                  +  7452) 
#define V00_IS_SENDFINISH_FLASHADDR          (MF_TEST19_FLASHADDR                  +  5184) 
#define V00_IS_SENDSTART_FLASHADDR           (V00_IS_SENDFINISH_FLASHADDR          +  7339) 

 
#endif
