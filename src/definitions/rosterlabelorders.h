#ifndef DEF_ROSTERLABELORDERS_H
#define DEF_ROSTERLABELORDERS_H

//Идентификаторы стандартных лэйблов
#define RLID_FOOTER_TEXT                          -5
#define RLID_DISPLAY                              -4
#define RLID_DECORATION                           -3
#define RLID_INDICATORBRANCH                      -2
#define RLID_NULL                                 -1

//Лэйбл выравнивается слева по центру
#define RLAP_LEFT_CENTER                          0
//Лэйбл выравнивается слева
#define RLAP_LEFT_TOP                             10000
//Лэйбл выравнивается справа
#define RLAP_RIGHT_TOP                            20000
//Лэйбл выравнивается справа по центру
#define RLAP_RIGHT_CENTER                         30000
//Лэйбл выравнивается по центру сверху
#define RLAP_CENTER_TOP                           40000
//Лэйбл выравнивается по центру
#define RLAP_CENTER_CENTER                        50000

/*************************************************************************/
#define RLO_DECORATION                            RLAP_LEFT_CENTER + 500

#define RLO_DISPLAY                               RLAP_LEFT_TOP + 500
#define RLO_GROUP_COUNTER                         RLAP_LEFT_TOP + 600

#define RLO_GATEWAY_ICON                          RLAP_RIGHT_CENTER + 500
#define RLO_AVATAR_IMAGE                          RLAP_RIGHT_CENTER + 501

#endif // DEF_ROSTERLABELORDERS_H
