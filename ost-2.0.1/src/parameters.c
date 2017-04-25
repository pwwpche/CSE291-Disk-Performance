
/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to this
 * license. If you do not agree to this license, do not download, install, copy
 * or use the software. Intel License Agreement
 *
 * Copyright (c) 2000, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * -Redistributions of source code must retain the above copyright notice, this
 *  list of conditions and the following disclaimer.
 *
 * -Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * -The name of Intel Corporation may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __KERNEL__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#endif
#include "util.h"
#include "debug.h"
#include "parameters.h"
#include "md5.h"

int param_list_add(ISCSI_PARAMETER_T **head, int type, char *key, 
                   char *dflt, char *valid) {
    ISCSI_PARAMETER_T *param;
  
    // Allocated new parameter type
 
    if (*head==NULL) {
        if ((*head=iscsi_malloc_atomic(sizeof(ISCSI_PARAMETER_T)))==NULL) {
            TRACE_ERROR("out of memory\n");
            return -1;
        }
        param = *head;
    } else {
        for (param=*head; param->next!=NULL; param=param->next);
        if ((param->next=iscsi_malloc_atomic(sizeof(ISCSI_PARAMETER_T)))
            ==NULL) {
            TRACE_ERROR("out of memory\n");
            return -1;
        }
        param = param->next;
    }

    // Initilized parameter

    param->type = type;                 // type
    strcpy(param->key, key);            // key
    strcpy(param->dflt, dflt);          // default value
    strcpy(param->valid, valid);        // list of valid values
    param->tx_offer = 0;                // sent offer
    param->rx_offer = 0;                // received offer
    param->tx_answer = 0;               // sent answer
    param->rx_answer = 0;               // received answer
    param->reset = 0;                   // used to erase value_l on next parse
    param->next = NULL;                 // terminate list

    // Allocated space for value list and set first item to default; and
    // set offer and answer lists to NULL

    if ((param->value_l=iscsi_malloc_atomic(sizeof(ISCSI_PARAMETER_VALUE_T)))
        ==NULL) {
        TRACE_ERROR("iscsi_malloc_atomic() failed\n");
        return -1;
    } 
    param->value_l->next = NULL;
    strcpy(param->value_l->value, dflt);

    // Arg check

    switch(type) {
    case ISCSI_PARAM_TYPE_DECLARATIVE:
        break;
    case ISCSI_PARAM_TYPE_DECLARE_MULTI:
        break;
    case ISCSI_PARAM_TYPE_BINARY_OR:
        if (strcmp(valid, "Yes,No")&&
            strcmp(valid, "No,Yes")&&
            strcmp(valid, "No")&&
            strcmp(valid, "Yes")&&
            strcmp(valid, "yes,no")&&
            strcmp(valid, "no,yes")&&
            strcmp(valid, "no")&&
            strcmp(valid, "yes")) {
            TRACE_ERROR("bad <valid> field \"%s\" for \
ISCSI_PARAM_TYPE_BINARY\n", 
                        valid);
            return -1;
        }
        break;
    case ISCSI_PARAM_TYPE_BINARY_AND:
        if (strcmp(valid, "Yes,No")&&
            strcmp(valid, "No,Yes")&&
            strcmp(valid, "No")&&
            strcmp(valid, "Yes")&&
            strcmp(valid, "yes,no")&&
            strcmp(valid, "no,yes")&&
            strcmp(valid, "no")&&
            strcmp(valid, "yes")) {
            TRACE_ERROR("bad <valid> field \"%s\" for \
ISCSI_PARAM_TYPE_BINARY\n", valid);
            return -1;
        }
        break;
    case ISCSI_PARAM_TYPE_NUMERICAL:
        break;
    case ISCSI_PARAM_TYPE_NUMERICAL_Z:
        break;
    case ISCSI_PARAM_TYPE_LIST:
        break;
    default:
        TRACE_ERROR("unknown parameter type %i\n", type);
        return -1;
    }

    TRACE(TRACE_PARAM, 1, 
          "\"%s\": valid \"%s\", default \"%s\", current \"%s\"\n", 
          param->key, param->valid, param->dflt, param->value_l->value)
        
        return 0;
}

int param_list_destroy(ISCSI_PARAMETER_T *head) {
    ISCSI_PARAMETER_T *ptr, *tmp;
    ISCSI_PARAMETER_VALUE_T *item_ptr, *next;

    for (ptr=head; ptr!=NULL;) {
        tmp = ptr;
        ptr = ptr->next;  
        if (tmp->value_l) {
            for (item_ptr=tmp->value_l; item_ptr!=NULL; item_ptr=next) {
                next = item_ptr->next;
                TRACE(TRACE_PARAM, 2, "freeing \"%s\" (%p)\n", 
                      item_ptr->value, item_ptr);
                iscsi_free_atomic(item_ptr);
            } 
        }
        TRACE(TRACE_PARAM, 1, "freeing %p\n", tmp);
        iscsi_free_atomic(tmp);
    }
    return 0;
}


ISCSI_PARAMETER_T *param_get(ISCSI_PARAMETER_T *head, char *key) {
    ISCSI_PARAMETER_T *ptr;

    for (ptr=head; ptr!=NULL; ptr=ptr->next) {
        if (!strcmp(ptr->key, key)) return ptr;
    }
    TRACE_ERROR("key \"%s\" not found in param list\n", key); 
    return NULL;
}

char* param_val(ISCSI_PARAMETER_T *head, char *key) {
    return param_val_which(head, key, 0);
}

char* param_val_which(ISCSI_PARAMETER_T *head, char *key, int which) {
    ISCSI_PARAMETER_T *ptr;
    ISCSI_PARAMETER_VALUE_T *item_ptr;
    int i = 0;

    for (ptr=head; ptr!=NULL; ptr=ptr->next) {
        if (!strcmp(ptr->key, key)) {
            item_ptr = ptr->value_l; 
            for (i=0; i!=which; i++) {
                if (item_ptr==NULL) {
                    return NULL;
                }
                item_ptr = item_ptr->next;
            }
            if (item_ptr==NULL) {
                return NULL;
            }
            return item_ptr->value;
        }
    }
    TRACE_ERROR("key \"%s\" not found in param list\n", key); 
    return NULL;
}

int param_val_delete_all(ISCSI_PARAMETER_T *head, char *key) {
    ISCSI_PARAMETER_T *ptr;
    ISCSI_PARAMETER_VALUE_T *item_ptr, *next;

    for (ptr=head; ptr!=NULL; ptr=ptr->next) {
        if (!strcmp(ptr->key, key)) {
            for (item_ptr=ptr->value_l; item_ptr!=NULL; item_ptr=next) {
                next = item_ptr->next;
                iscsi_free_atomic(item_ptr);
            } 
            ptr->value_l = NULL;
            return 0;
        }
    }
    TRACE_ERROR("key \"%s\" not found in param list\n", key); 
    return -1;
}

int param_val_reset(ISCSI_PARAMETER_T *head, char *key) {
    ISCSI_PARAMETER_T *ptr;

    for (ptr=head; ptr!=NULL; ptr=ptr->next) {
        if (!strcmp(ptr->key, key)) {
            ptr->reset = 1;
            return 0;
        }
    }
    TRACE_ERROR("key \"%s\" not found in param list\n", key); 
    return -1;
}

int param_atoi(ISCSI_PARAMETER_T *head, char *key) {
    ISCSI_PARAMETER_T *ptr;
    char *value;

    for (ptr=head; ptr!=NULL; ptr=ptr->next) {
        if (!strcmp(ptr->key, key)) {
            if (ptr->value_l) {
                if ((value=param_val(head, key))!=NULL) {
                    return iscsi_atoi(value);
                } else {
                    TRACE_ERROR("value is NULL\n");
                    return 0;
                }
            } else {
                TRACE_ERROR("param \"%s\" has NULL value list\n", key); 
                return 0;
            }
        }
    }
    TRACE_ERROR("key \"%s\" not found in param list\n", key); 
    return 0; 
}

int param_equiv(ISCSI_PARAMETER_T *head, char *key, char *val) {
    ISCSI_PARAMETER_T *ptr;
    char *value;

    for (ptr=head; ptr!=NULL; ptr=ptr->next) {
        if (!strcmp(ptr->key, key)) {
            if (ptr->value_l) {
                if ((value=param_val(head, key))==NULL) {
                    TRACE_ERROR("key \"%s\" value is NULL\n", key);
                    return -1;
                } else {
                    if (!strcmp(value, val)) {
                        return 0;
                    } else {
                        return 1;
                    }
                }
            } else {
                TRACE_ERROR("param \"%s\" has NULL value list\n", key);
                return -1;
            }
        }
    }
    TRACE_ERROR("key \"%s\" not found in param list\n", key); 
    return -1;
}

int param_num_vals(ISCSI_PARAMETER_T *head, char *key) {
    ISCSI_PARAMETER_T *ptr;
    ISCSI_PARAMETER_VALUE_T *item_ptr;
    int num = 0;

    for (ptr=head; ptr!=NULL; ptr=ptr->next) {
        if (!strcmp(ptr->key, key)) {
            for (item_ptr=ptr->value_l; item_ptr!=NULL; 
                 item_ptr=item_ptr->next) {
                num++;
            }
            return num;
        }
    }
    TRACE_ERROR("key \"%s\" not found in param list\n", key); 
    return -1;
}

int param_list_print(ISCSI_PARAMETER_T *head) {
    ISCSI_PARAMETER_T *ptr;
    ISCSI_PARAMETER_VALUE_T *item_ptr;

    for (ptr=head; ptr!=NULL; ptr=ptr->next) {
        for (item_ptr=ptr->value_l; item_ptr!=NULL; item_ptr=item_ptr->next) {
            PRINT("\"%s\"=\"%s\"\n", ptr->key, item_ptr->value);
        }
    }
    return 0;
}

int param_text_print(ISCSI_PARAMETER_T *head, char *text, unsigned text_len) {
    char key[256];
    char *ptr, *delim_ptr, *value;

    for (ptr=text; ptr-text<text_len; ptr+=(strlen(ptr)+1)) {

        /* Skip over any NULLs */
        while (!(*ptr)&&((ptr-text)<text_len)) ptr++;
        if ((ptr-text)>=text_len) break;

        if ((delim_ptr=strchr(ptr, '='))==NULL) {
            TRACE_ERROR("delimiter \'=\' not found in token \"%s\"\n", ptr);
            return -1;
        }
        strncpy(key, ptr, delim_ptr-ptr); key[delim_ptr-ptr] = '\0';
        value = delim_ptr+1;
#if 0
        PRINT(
              "\"%s\"=\"%s\" (offer %i answer %i rx_offer %i rx_answer %i)\n", 
              key, value, 
              param_get(head, key)->tx_offer,
              param_get(head, key)->tx_answer,
              param_get(head, key)->rx_offer,
              param_get(head, key)->rx_answer);
#else
        TRACE_CLEAN(TRACE_PARAM, 0, "\"%s\"=\"%s\"\n", key, value);
#endif
    }
    return 0;
}

int param_text_add(ISCSI_PARAMETER_T *head, char *key, char *value, 
                   char *text, int *len, int offer) {
    if ((*len+strlen(key)+1+strlen(value)+1) > ISCSI_PARAM_MAX_TEXT_LEN) {
        TRACE_ERROR("error adding key \"%s\" -- max text data size is %u\n", 
                    key, ISCSI_PARAM_MAX_TEXT_LEN);
        return -1;
    }
    sprintf(text+*len, "%s=%s", key, value);
    *len += strlen(text+*len)+1;
    return 0;
}

int driver_atoi(const char *s)
{
    int k = 0;
    while (*s != '\0' && *s >= '0' && *s <= '9') {
        k = 10 * k + (*s - '0');
        s++;
    }
    return k;
}

/* Security offering and check */
/* ret values:
   =0: succeed or no security
   >0: security negotiation in process
   <0: failed
*/ 

typedef struct {
    unsigned char idData[1];
    unsigned char chapdata[ISCSI_CHAP_C_MAX_DATA_LENGTH];
    unsigned char respdata[ISCSI_CHAP_R_DATA_LENGTH];
} SECURITY_COOKIE_T;

#define PPS_CLEANUP { if (chapstring != NULL) iscsi_free(chapstring);\
        if (context != NULL) iscsi_free(context); }
#define PPS_ERROR { PPS_CLEANUP; return (-1); };

int param_parse_security(ISCSI_PARAMETER_T *head, ISCSI_PARAMETER_T *param_in, 
                         char *text_out, int *text_len_out, void **ptr,
                         char *InitiatorUser, char *InitiatorPass, 
                         char *TargetUser, char *TargetPass) {
    unsigned char *idData;
    unsigned char *chapdata;
    unsigned char *respdata;
    char *chapstring = NULL;
    MD5Context_t *context = NULL;
    ISCSI_PARAMETER_T *param = NULL;
    int ret = 1 ;
    SECURITY_COOKIE_T *cookie;

    if (*ptr == NULL) {
        if ((*ptr = iscsi_malloc_atomic(sizeof(SECURITY_COOKIE_T)))==NULL) {
            TRACE_ERROR("iscsi_malloc_atomic() failed\n");
            return -1;
        }               
    }
    cookie = (SECURITY_COOKIE_T *)(*ptr);
    idData = cookie->idData;
    chapdata = cookie->chapdata;
    respdata = cookie->respdata;

    if ((chapstring=iscsi_malloc(ISCSI_CHAP_STRING_LENGTH))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        return -1;
    }

    if ((context=iscsi_malloc(sizeof(MD5Context_t)))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        if (chapstring != NULL) iscsi_free(chapstring);
        return -1;
    }

    if(strcmp(param_in->key, "AuthMethod") == 0) {
        if(param_in->rx_answer)
            if (0==strcmp(param_in->answer_rx, "None")){
                PPS_CLEANUP;
                return 0; /* Proposed None for Authentication */
            }
        if(param_in->rx_offer)
            if (0==strcmp(param_in->offer_rx, "None")) {
                PPS_CLEANUP;
                return 0;
            }
        if (!param_in->rx_offer) {
            param= param_get(head, "CHAP_A");
            if (param == NULL) PPS_ERROR;
            param->tx_offer = 1;   // sending an offer
            param->rx_offer = 0;   // reset
            strcpy(param->offer_tx, param->valid); 
            PARAM_TEXT_ADD_ELSE(head, param->key, param->valid,
                                text_out, text_len_out, 0, PPS_ERROR);
            ret++;
        }

    } else if (strcmp(param_in->key, "CHAP_A") == 0) {
        if (param_in->rx_offer) {

            /* target */
            PARAM_TEXT_ADD_ELSE(head, param_in->key, param_in->offer_rx,
                                text_out, text_len_out, 0, PPS_ERROR);  
            param= param_get(head, "CHAP_I");
            if (param == NULL) PPS_ERROR; 
            param->tx_offer = 0;   // sending an offer
            param->rx_offer = 0;   // reset
            GenRandomData(idData, 1);
            sprintf(chapstring, "%d", idData[0]);
            strcpy(param->offer_tx, chapstring); 
            PARAM_TEXT_ADD_ELSE(head, param->key, param->offer_tx,
                                text_out, text_len_out, 0, PPS_ERROR);

            param= param_get(head, "CHAP_C");
            if (param == NULL) PPS_ERROR;
            param->tx_offer = 0;   // sending an offer
            param->rx_offer = 0;   // reset
            GenRandomData(chapdata, ISCSI_CHAP_C_DATA_LENGTH_T);
            if (HexDataToText(chapdata, ISCSI_CHAP_C_DATA_LENGTH_T,
                              chapstring, ISCSI_CHAP_STRING_LENGTH)!=0) {
                TRACE_ERROR("error coverting chap data to text\n");
                PPS_ERROR;
            }
            strcpy(param->offer_tx, chapstring); 
            PARAM_TEXT_ADD_ELSE(head, param->key, param->offer_tx,
                                text_out, text_len_out, 0, PPS_ERROR);
            ret++;
        } else {
            /* initiator */
        }       
    } else if (strcmp(param_in->key, "CHAP_I") == 0) {
        if (param_in->rx_offer) {
            /* initiator */         
            TRACE(TRACE_PARAM, 1, "initiator: CHAP_I (offer %s)\n", 
                  param_in->offer_rx);
            idData[0] = driver_atoi(param_in->offer_rx);
        } else {
            /* target */
            TRACE(TRACE_PARAM, 1, "target: CHAP_I (answer %s)\n", 
                  param_in->answer_rx);
            idData[0] = driver_atoi(param_in->answer_rx);
        }
        ret++;
    } else if (strcmp(param_in->key, "CHAP_C") == 0) {    
        int chapdata_len;

        if (param_in->rx_offer) {
            /* initiator */
            TRACE(TRACE_PARAM, 0, "initiator: CHAP_C (offer %s, len %Zu)\n", 
                  param_in->offer_rx, strlen(param_in->offer_rx));
            chapdata_len = HexTextToData(param_in->offer_rx, 
                                         strlen(param_in->offer_rx), chapdata, 
                                         ISCSI_CHAP_C_MAX_DATA_LENGTH);
        } else {
            /* target */
            TRACE(TRACE_PARAM, 0, "target: CHAP_C (answer %s, len %Zu)\n", 
                  param_in->answer_rx, strlen(param_in->offer_rx));
            chapdata_len = HexTextToData(param_in->answer_rx, 
                                         strlen(param_in->answer_rx), 
                                         chapdata, ISCSI_CHAP_C_DATA_LENGTH_T);
        }
        if (chapdata_len <=0) {
            TRACE_ERROR("error converting CHAP string to data\n");
            PPS_ERROR;
        }

        /* username */
        param= param_get(head, "CHAP_N");
        if (param == NULL) PPS_ERROR;
        param->tx_offer = 1;   // sending an offer
        param->rx_offer = 0;   // reset
        if (param_in->rx_offer) {
            strcpy(param->offer_tx, InitiatorUser); 
        } else {
            strcpy(param->offer_tx, TargetUser);
        }       
        PARAM_TEXT_ADD_ELSE(head, param->key, param->offer_tx, text_out, 
                            text_len_out, 0, PPS_ERROR);

        /* response to challenge */
        param= param_get(head, "CHAP_R");
        if (param == NULL) PPS_ERROR;
        param->tx_offer = 1;   // sending an offer
        param->rx_offer = 0;   // reset
        MD5Init(context);
        MD5Update(context, idData, 1);
        if (param_in->rx_offer) {
            MD5Update(context, (md5byte *) InitiatorPass, 
                      strlen(InitiatorPass));
        } else {
            MD5Update(context, (md5byte *) TargetPass, strlen(TargetPass));
        }
        MD5Update(context, chapdata, chapdata_len);
        MD5Final(chapdata, context);
        HexDataToText(chapdata, ISCSI_CHAP_R_DATA_LENGTH, param->offer_tx, 
                      ISCSI_CHAP_STRING_LENGTH);
        PARAM_TEXT_ADD_ELSE(head, param->key, param->offer_tx, text_out, 
                            text_len_out, 0, PPS_ERROR);

        /* ask the target to authenticate if ISCSI_CHAP_USER is set */
        if ((param_in->rx_offer)&&strlen(TargetUser)) {

            param= param_get(head, "CHAP_I");
            if (param == NULL) PPS_ERROR;
            param->tx_offer = 0;   // sending an offer
            param->rx_offer = 0;   // reset
            GenRandomData(idData, 1);
            sprintf(chapstring, "%d", idData[0]);
            strcpy(param->offer_tx, chapstring); 
            PARAM_TEXT_ADD_ELSE(head, param->key, param->offer_tx, text_out, 
                                text_len_out, 0, PPS_ERROR);

            param= param_get(head, "CHAP_C");
            if (param == NULL) PPS_ERROR;
            param->tx_offer = 0;   // sending an offer
            param->rx_offer = 0;   // reset
            GenRandomData(chapdata, ISCSI_CHAP_C_DATA_LENGTH_I);
            HexDataToText(chapdata, ISCSI_CHAP_C_DATA_LENGTH_I, chapstring, 
                          ISCSI_CHAP_STRING_LENGTH);
            strcpy(param->offer_tx, chapstring); 
            PARAM_TEXT_ADD_ELSE(head, param->key, param->offer_tx, text_out, 
                                text_len_out, 0, PPS_ERROR);     
        }
        ret++;
        
    } else if (strcmp(param_in->key, "CHAP_N") == 0) {
        if (param_in->rx_answer) {
            /* initiator */
            if (strcmp(param_in->answer_rx, TargetUser) != 0) {
                TRACE_ERROR("Unknown user \"%s\" (expecting \"%s\")\n", 
                            param_in->answer_rx, TargetUser);
                PPS_ERROR;
            }
        } else {
            /* target */
            if (strcmp(param_in->offer_rx, InitiatorUser) != 0) {
                TRACE_ERROR("Unknown user \"%s\" (expecting \"%s\")\n", 
                            param_in->offer_rx, InitiatorUser);
                PPS_ERROR;
            }
        }
        ret++;
    } else if (strcmp(param_in->key, "CHAP_R") == 0) {
        MD5Init(context);       
        MD5Update(context, idData, 1);
        HexDataToText(idData, 1, param_in->offer_tx, ISCSI_CHAP_STRING_LENGTH);
        if (param_in->rx_offer) {
            /* target */
            HexDataToText(chapdata, ISCSI_CHAP_C_DATA_LENGTH_T,
                          chapstring, ISCSI_CHAP_STRING_LENGTH);
            MD5Update(context, (md5byte *) InitiatorPass, 
                      strlen(InitiatorPass));
            MD5Update(context, chapdata, ISCSI_CHAP_C_DATA_LENGTH_T);
        } else {
            /* initiator */
            HexDataToText(chapdata, ISCSI_CHAP_C_DATA_LENGTH_I,
                          chapstring, ISCSI_CHAP_STRING_LENGTH);
            MD5Update(context, (md5byte *) TargetPass, strlen(TargetPass));
            MD5Update(context, chapdata, ISCSI_CHAP_C_DATA_LENGTH_I);
        }
        MD5Final(chapdata, context);
        if (param_in->rx_offer) {
            HexTextToData(param_in->offer_rx, strlen(param_in->offer_rx),
                          respdata, ISCSI_CHAP_R_DATA_LENGTH);
        } else {
            HexTextToData(param_in->answer_rx, strlen(param_in->answer_rx),
                          respdata, ISCSI_CHAP_R_DATA_LENGTH);
        }       
        HexDataToText(chapdata, ISCSI_CHAP_R_DATA_LENGTH,
                      param_in->offer_rx, ISCSI_CHAP_STRING_LENGTH);
        PRINT_BUFF(TRACE_ISCSI, 1, respdata, ISCSI_CHAP_R_DATA_LENGTH, 32);
        PRINT_BUFF(TRACE_ISCSI, 1, chapdata, ISCSI_CHAP_R_DATA_LENGTH, 32);
        if (memcmp(respdata, chapdata, ISCSI_CHAP_R_DATA_LENGTH) != 0) {
            if (param_in->rx_offer) {   
                TRACE_ERROR("Failed to authentication initiator "
                            "(initiator should use password \"%s\")\n", 
                            InitiatorPass);
            } else {
                TRACE_ERROR("Failed to authentice target (target "
                            "should use password \"%s\")\n", TargetPass);
            }
            PPS_ERROR;
        }
        else
            PPS_CLEANUP;
        return 0;
        ret++;
    }
    PPS_CLEANUP;
    return (ret);
}

#define PTP_CLEANUP { if (key != NULL) iscsi_free(key); \
        if (offer != NULL) iscsi_free(offer);           \
        if (valid != NULL) iscsi_free(valid);           \
        if (val1 != NULL) iscsi_free(val1);             \
        if (val2 != NULL) iscsi_free(val2);             \
        if (tmp_key != NULL) iscsi_free(tmp_key); }

#define PTP_ERROR {PTP_CLEANUP; return -1;}
#define PTP_WARN {PTP_CLEANUP; return 0;}

int param_text_parse(ISCSI_PARAMETER_T *head, char *text_in,  int  text_len_in, 
                     char *text_out, int *text_len_out, int outgoing, 
                     void **cookie, char *InitiatorUser, char *InitiatorPass, 
                     char *TargetUser, char *TargetPass) {
    char *key = NULL;
    char *value = NULL;
    char *ptr, *delim_ptr;
    ISCSI_PARAMETER_T *param;
    ISCSI_PARAMETER_VALUE_T *item_ptr;
    int offer_i, answer_i, max_i, val1_i, val2_i, negotiated_i;
    char *p1, *p2, *p3, *p4;
    char *offer = NULL;
    char *valid = NULL;
    char *val1 = NULL;
    char *val2 = NULL;
    char *tmp_key = NULL;
    char c;
    int ret;
    static int i = -1;

    /* Whether incoming or outgoing, some of the params might be offers
       and some answers. Incoming text has the potential for creating
       outgoing text - and this will happen when the incoming text has
       offers that need an answer*/
 
    i++;

    TRACE_CLEAN(TRACE_PARAM, 0, "=== BEGIN %s ===\n",  
                outgoing?"OUTGOING":"INCOMING");
    param_text_print(head, text_in, text_len_in);
    TRACE_CLEAN(TRACE_PARAM, 0, "=== END %s ===\n", 
                outgoing?"OUTGOING":"INCOMING");

    if ((key=iscsi_malloc(ISCSI_PARAM_KEY_LEN))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        return -1;
    }
    if ((offer=iscsi_malloc(ISCSI_PARAM_MAX_LEN))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        if (key != NULL) iscsi_free(key);
        return -1;
    }
    if ((valid=iscsi_malloc(ISCSI_PARAM_MAX_LEN))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        if (key != NULL) iscsi_free(key);
        if (offer != NULL) iscsi_free(offer);
        return -1;
    }
    if ((val1=iscsi_malloc(ISCSI_PARAM_MAX_LEN))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        if (key != NULL) iscsi_free(key);
        if (offer != NULL) iscsi_free(offer);
        if (valid != NULL) iscsi_free(valid);
        return -1;
    }
    if ((val2=iscsi_malloc(ISCSI_PARAM_MAX_LEN))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        if (key != NULL) iscsi_free(key);
        if (offer != NULL) iscsi_free(offer);
        if (valid != NULL) iscsi_free(valid);
        if (val1 != NULL) iscsi_free(val1);
        return -1;
    }
    
    if (!outgoing) *text_len_out = 0;
    
    TRACE_CLEAN(TRACE_PARAM, 0, "********************************************"
                "******\n");
    TRACE_CLEAN(TRACE_PARAM, 0, "*              PARAMETERS NEGOTIATED        "
                "     *\n");
    TRACE_CLEAN(TRACE_PARAM, 0, "*                                           "
                "     *\n");
    
    for (ptr=text_in; ptr-text_in<text_len_in; ptr+=(strlen(ptr)+1)) {

        /* Skip over any NULLs */       
        while (!(*ptr)&&((ptr-text_in)<text_len_in)) ptr++;
        if ((ptr-text_in)>=text_len_in) break;

        /* Extract <key>=<value> token from text_in */
        if ((delim_ptr=strchr(ptr, '='))==NULL) {
            TRACE_ERROR("delimiter \'=\' not found in token \"%s\"\n", ptr);
        } else {
            if((delim_ptr-ptr) >= (ISCSI_PARAM_KEY_LEN-1)) {
                if( !outgoing ) {
                    tmp_key = iscsi_malloc(delim_ptr-ptr);
                    if(tmp_key) {
                        strncpy(tmp_key,  ptr, delim_ptr-ptr);
                        tmp_key[delim_ptr-ptr] = '\0';
                        PARAM_TEXT_ADD_ELSE(head, tmp_key, "NotUnderstood", 
                                            text_out, text_len_out, 0, 
                                            PTP_ERROR);
                    }
                } else {
                    PRINT("ignoring \"%s\"\n", key);
                }
                goto next;
            }
            strncpy(key, ptr, delim_ptr-ptr); key[delim_ptr-ptr] = '\0';
            value = delim_ptr+1;
        }
        
        /* Find key in param list */    
        for (param=head; param!=NULL; param=param->next) {
            if (strcmp(param->key, key)==0) break;
        }
        if (param==NULL) {
            if( !outgoing ) {
                PARAM_TEXT_ADD_ELSE(head, key, "NotUnderstood", text_out, 
                                    text_len_out, 0, PTP_ERROR);
            } else {
                PRINT("ignoring \"%s\"\n", key);
            }
            goto next;
        } 
        RETURN_GREATER("strlen(value)", (unsigned int) strlen(value), 
                       ISCSI_PARAM_MAX_LEN, PTP_CLEANUP, -1); 
        
        /* We're sending|receiving an offer|answer */   
        if (outgoing) {
            if (param->rx_offer) {   
                param->tx_answer = 1;  // sending an answer
                strcpy(param->answer_tx, value); 
                TRACE(TRACE_PARAM, 1, 
                      "sending answer \"%s\"=\"%s\" for offer \"%s\"\n", 
                      param->key, param->answer_tx, param->offer_rx); 
                goto negotiate;
            } else {
                param->tx_offer = 1;   // sending an offer
                param->rx_offer = 0;   // reset
                strcpy(param->offer_tx, value); 
                TRACE(TRACE_PARAM, 1, 
                      "sending offer \"%s\"=\"%s\"\n", 
                      param->key, param->offer_tx);  
                if ((param->type == ISCSI_PARAM_TYPE_DECLARATIVE)||
                    (param->type == ISCSI_PARAM_TYPE_DECLARE_MULTI)) {
                    goto negotiate;
                }
                goto next;
            }
        } else {
            if (param->tx_offer) {   
                param->rx_answer = 1;  // received an answer
                param->tx_offer = 0;   // reset
                strcpy(param->answer_rx, value); 
                TRACE(TRACE_PARAM, 1, 
                      "received answer \"%s\"=\"%s\" for offer \"%s\"\n", 
                      param->key, param->answer_rx, param->offer_tx); 
                if ((ret=param_parse_security(head, param, text_out, 
                                              text_len_out, cookie, 
                                              InitiatorUser, InitiatorPass, 
                                              TargetUser, TargetPass))<0) {
                    TRACE_ERROR("param_parse security status is %i (EXITING)\n",
                                ret);
                    PTP_CLEANUP;
                    return (ISCSI_PARAM_STATUS_AUTH_FAILED);                
                }
                goto negotiate;

#if 0
                if ((ret = param_parse_security(head, param, text_out, 
                                                text_len_out, cookie)) > 1) {
                    PRINT("param_parse security status is %i (goto "
                          "negotiate)\n", ret);
                    goto negotiate;
                } else if (ret == 0) {
                    PRINT("param_parse security status is %i (goto \
negotiate)\n", ret);

                    /* FIX ME 
                       Happens in initiator code 
                       currently we ignore initiator authentication status
                       See comments at the beginning of parse_security
                    */

                    goto negotiate;
                } else if (ret == 1) {
                    PRINT("param_parse security status is %i (goto "
                          "negotiate)\n", ret);
                    goto negotiate;
                } else { 
                    PRINT("param_parse security status is %i (EXITING)\n", ret);
                    PTP_CLEANUP;
                }
                return (ISCSI_PARAM_STATUS_AUTH_FAILED);
#endif

            } else {
                param->rx_offer = 1;   // received an offer
                strcpy(param->offer_rx, value); 
                TRACE(TRACE_PARAM, 1, "received offer \"%s\"=\"%s\"\n", 
                      param->key, param->offer_rx); 
                if ((ret = param_parse_security(head, param, text_out, 
                                                text_len_out, cookie, 
                                                InitiatorUser, 
                                                InitiatorPass, TargetUser, 
                                                TargetPass)) > 1)
                    goto next;
                else if (ret < 0) {
                    ISCSI_PARAMETER_T *auth_result;
                    if ((auth_result = param_get(head, "AuthResult"))) {
                        strcpy(auth_result->value_l->value, "Fail");
                    }
                    PTP_CLEANUP;
                    return (ISCSI_PARAM_STATUS_AUTH_FAILED);
                }
                else if (ret==0) {
                    ISCSI_PARAMETER_T *auth_result;

                    if ((auth_result = param_get(head, "AuthResult"))) {
                        strcpy(auth_result->value_l->value, "Yes");
                    }
                }

                /* Answer offer if an inquiry or type is not DECLARATIVE */
                
                if ((strcmp(param->offer_rx, "?")!=0) && 
                    ((param->type == ISCSI_PARAM_TYPE_DECLARATIVE)||
                     (param->type == ISCSI_PARAM_TYPE_DECLARE_MULTI))) {
                    goto negotiate;
                } else {
                    goto answer;
                }
            }
        }
        
    answer:
        
        // Answer with current value if this is an inquiry (<key>=?)
        
        if (!strcmp(value, "?")) {
            TRACE(TRACE_PARAM, 1, "got inquiry for param \"%s\"\n", param->key);
            if (param->value_l) {
                if (param->value_l->value) {
                    strcpy(param->answer_tx, param->value_l->value);
                } else {
                    TRACE_ERROR("param \"%s\" has NULL value_l->value\n", 
                                param->key);
                    strcpy(param->answer_tx, "");
                }
            } else {
                TRACE_ERROR("param \"%s\" has NULL value_l\n", param->key);
                strcpy(param->answer_tx, "");
            }
            goto add_answer;
        } 
        
        // Generate answer according to the parameter type
        
        switch(param->type) {
            
        case ISCSI_PARAM_TYPE_BINARY_AND:
            goto binary_or;
            
        case ISCSI_PARAM_TYPE_BINARY_OR:
        binary_or: if (strcmp(value, "yes")&&strcmp(value, "no")
                       &&strcmp(value, "Yes")&&strcmp(value, "No")) {
                TRACE_ERROR("\"%s\" is not a valid binary value\n", value);
                strcpy(param->answer_tx, "Reject");
                goto add_answer;
            } 
            if (strchr(param->valid, ',')!=NULL) {
                // we accept both yes and no, so answer w/ their offer
                strcpy(param->answer_tx, value);
            } else {
                // answer with the only value we support
                strcpy(param->answer_tx, param->valid); 
            }
            break;

        case ISCSI_PARAM_TYPE_LIST:

#if 0
            /*
             * Use our default value if it's offered as one of the option
             * in the parameter list.
             *
             * We need to do this at least for CHAP because cisco's initiator
             * could be sending us a parameter value list with "CHAP,None",
             * even when it doesn't set username/password in its configration
             * file, in which case we should pick "None" as for no security 
             * instead of pick the first one on the value list. "None" is the
             * default value for AuthMethod
             *
             * This fix is working well now, though is arguable. We should keep
             * this just to make us work with Cisco for now.
             */
            if (strlen(param->dflt)) {
                for (p1=p2=param->offer_rx; p2; p1=p2+1) {
                    
                    if ((p2=strchr(p1, ','))) {
                        strncpy(offer, p1, p2-p1);
                        offer[p2-p1] = '\0';
                    } else {
                        strcpy(offer, p1);
                    }
                    
                    if (!strcmp(param->dflt, offer)) {
                        strcpy(param->answer_tx, offer);
                        goto add_answer;
                    }
                }
            }
#endif
            
            // Find the first valid offer that we support
            
            for (p1=p2=param->offer_rx; p2; p1=p2+1) {
                if ((p2=strchr(p1, ','))) {
                    strncpy(offer, p1, p2-p1);
                    offer[p2-p1] = '\0';
                } else {
                    strcpy(offer, p1);
                }
                if (strlen(param->valid)) {
                    for (p3=p4=param->valid; p4; p3=p4+1) {
                        if ((p4=strchr(p3, ','))) {
                            strncpy(valid, p3, p4-p3);
                            valid[p4-p3] = '\0';
                        } else {
                            strcpy(valid, p3);
                        }
                        if (!strcmp(valid, offer)) {
                            strcpy(param->answer_tx, offer);
                            goto add_answer;
                        }
                    }
                } else {
                    TRACE(TRACE_PARAM, 1, 
                          "Valid list empty. Answering with first in offer "
                          "list\n");
                    strcpy(param->answer_tx, offer);
                    goto add_answer;
                } 
                TRACE(TRACE_PARAM, 1,
                      "\"%s\" is not a valid offer for key \"%s\" "
                      "(must choose from \"%s\")\n", 
                      offer, param->key, param->valid);
            }
            TRACE(TRACE_PARAM, 1, 
                  "No Valid offers: \"%s\" is added as value for key \"%s\")\n",
                  "Reject", 
                  param->key);
            strcpy(param->answer_tx, "Reject");
            break;
            
        case ISCSI_PARAM_TYPE_NUMERICAL_Z:
            goto numerical;
            
        case ISCSI_PARAM_TYPE_NUMERICAL:
        numerical: 
            offer_i = iscsi_atoi(param->offer_rx);
            max_i = iscsi_atoi(param->valid);
            if (param->type == ISCSI_PARAM_TYPE_NUMERICAL_Z) {
                if (max_i == 0) {
                    answer_i = offer_i;     // return whatever they offered
                } else if (offer_i == 0) {
                    answer_i = max_i;       // return only what we can support
                } else if (offer_i>max_i) {
                    answer_i = max_i;       // we are the lower of the two
                } else {
                    answer_i = offer_i;     // they are the lower of the two
                }
            } else {
                if (offer_i>max_i) {
                    answer_i = max_i;       // we are the lower of the two
                } else {
                    answer_i = offer_i;     // they are the lower of the two
                }
            }
            sprintf(param->answer_tx, "%i", answer_i);
            goto add_answer;
            
        default:
            goto next;
        }

    add_answer: 

        PARAM_TEXT_ADD_ELSE(head, key, param->answer_tx, text_out, 
                            text_len_out, 0, PTP_ERROR);
        TRACE(TRACE_PARAM, 1, "answering \"%s\"=\"%s\"\n", 
              param->key, param->answer_tx);
        goto next;
        
        // Negotiate after receiving|sending an answer 
        
    negotiate: 
        
        TRACE(TRACE_PARAM, 0, "negotiating %s (type %i, offer_tx \"%s\" "
              "offer_rx \"%s\" answer_tx \"%s\" answer_rx \"%s\")\n", 
              param->key, param->type, param->offer_tx, 
              param->offer_rx, param->answer_tx, param->answer_rx);

        switch(param->type) {
        case ISCSI_PARAM_TYPE_DECLARE_MULTI:
            goto declarative_negotiate;
        case ISCSI_PARAM_TYPE_DECLARATIVE:
        declarative_negotiate:
            if (outgoing) {
                strcpy(param->negotiated, param->offer_tx);
            } else {
                if (param->rx_offer) {
                    strcpy(param->negotiated, param->offer_rx);
                } else {
                    strcpy(param->negotiated, param->answer_rx);
                }
            }
            break;
        case ISCSI_PARAM_TYPE_BINARY_AND:
            goto binary_or_negotiate;
        case ISCSI_PARAM_TYPE_BINARY_OR:
        binary_or_negotiate:
            if (outgoing) {
                strcpy(val1, param->offer_rx); 
                strcpy(val2, param->answer_tx);
            } else {
                strcpy(val1, param->answer_rx); 
                strcpy(val2, param->offer_tx);
                // Make sure the answer is valid
                if( strcmp(val1, "Yes") && strcmp(val1, "No") &&
                    strcmp(val1, "yes") && strcmp(val1, "no") &&
                    strcmp(val1, "Irrelevant")) {
                    // Invalid value returned as answer.
                    TRACE_ERROR("Invalid answer (%s) for key (%s)\n",
                                val1, key);
                    PTP_WARN;
                    PTP_ERROR;
                }
            }
            if (param->type == ISCSI_PARAM_TYPE_BINARY_OR) { 
                if (!strcmp(val1, "yes")||!strcmp(val2, "yes")
                    ||!strcmp(val1, "Yes")||!strcmp(val2, "Yes")) {
                    strcpy(param->negotiated, "Yes");
                } else {
                    strcpy(param->negotiated, "No");
                }
            } else {
                if ((!strcmp(val1, "yes")||!strcmp(val1, "Yes"))
                    &&(!strcmp(val2, "yes")||!strcmp(val2, "Yes"))) {
                    strcpy(param->negotiated, "Yes");
                } else {
                    strcpy(param->negotiated, "No");
                }
            }
            break;
        case ISCSI_PARAM_TYPE_NUMERICAL_Z:
            goto numerical_negotiate;
        case ISCSI_PARAM_TYPE_NUMERICAL:
        numerical_negotiate:
            if (outgoing) {
                strcpy(val1, param->offer_rx); 
                strcpy(val2, param->answer_tx);
            } else {
                strcpy(val1, param->answer_rx); 
                strcpy(val2, param->offer_tx);
            }
            val1_i = iscsi_atoi(val1);
            val2_i = iscsi_atoi(val2);
            if (param->type == ISCSI_PARAM_TYPE_NUMERICAL_Z) {
                if (val1_i == 0) {
                    negotiated_i = val2_i;  
                } else if (val2_i == 0) {
                    negotiated_i = val1_i;  
                } else if (val1_i>val2_i) {
                    negotiated_i = val2_i;
                } else {
                    negotiated_i = val1_i; 
                }
            } else {
                if (val1_i>val2_i) {
                    negotiated_i = val2_i;
                } else {
                    negotiated_i = val1_i;
                }
            }
            sprintf(param->negotiated, "%i", negotiated_i);
            break;
        case ISCSI_PARAM_TYPE_LIST:
            if (outgoing) {
                if (param->tx_offer) {
                    TRACE_ERROR("we should not be here\n");
                    PTP_ERROR;
                } else if (param->tx_answer) {
                    strcpy(val1, param->answer_tx);
                } else {
                    TRACE_ERROR("unexpected error\n");
                    PTP_ERROR;
                }
            } else {
                if (param->rx_offer) {
                    TRACE_ERROR("we should not be here\n");
                    PTP_ERROR;
                } else if (param->rx_answer) {
                    strcpy(val1, param->answer_rx);
                } else {
                    TRACE_ERROR("unexpected error\n");
                    PTP_ERROR;
                }
            }
            
            // Make sure incoming or outgoing answer is valid
            // None, Reject, Irrelevant and NotUnderstood are valid 
            if ((!strcmp(val1, "None")) || (!strcmp(val1, "Reject")) ||
                (!strcmp(val1, "Irrelevant")) || 
                (!strcmp(val1, "NotUnderstood"))) {
                goto value_ok;
            }  
            
            if (strlen(param->valid)) {
                for (p3=p4=param->valid; p4; p3=p4+1) {
                    if ((p4=strchr(p3, ','))) {
                        strncpy(valid, p3, p4-p3);
                        valid[p4-p3] = '\0';
                    } else {
                        strcpy(valid, p3);
                    }
                    if (!strcmp(valid, val1)) {
                        goto value_ok;
                    }
                }
            } else {
                TRACE(TRACE_PARAM, 1, "Valid list empty??\n");
                PTP_ERROR;
            } 
            TRACE_ERROR("\"%s\" is not a valid value (must choose from "
                        "\"%s\")\n", val1, param->valid);
            PTP_ERROR;
        value_ok: strcpy(param->negotiated, val1);
            break;
        }
        
        TRACE(TRACE_PARAM, 0, "negotiated \"%s\"=\"%s\"\n", 
              param->key, param->negotiated); 
        
        // For inquiries, we don't commit the value.
        
        if (param->tx_offer&&!strcmp(param->offer_tx, "?")) {
            // we're offering an inquiry 
            TRACE(TRACE_PARAM, 1, "sending an inquiry for \"%s\"\n", 
                  param->key); 
            goto next;
        } else if (param->rx_offer&&!strcmp(param->offer_rx, "?")) {
            // we're receiving an inquiry 
            TRACE(TRACE_PARAM, 1, "received an inquiry for \"%s\"\n", 
                  param->key); 
            goto next;
        } else if (param->tx_answer&&!strcmp(param->offer_rx, "?")) {
            // we're answering an inquiry 
            TRACE(TRACE_PARAM, 1, "answering an inquiry for \"%s\"\n", 
                  param->key); 
            goto next;
        } else if (param->rx_answer&&!strcmp(param->offer_tx, "?")) {
            // we're receiving an answer for our inquiry 
            TRACE(TRACE_PARAM, 1, "received an answer for inquiry on \"%s\"\n",
                  param->key); 
            goto next;
        } 
        
        TRACE(TRACE_PARAM, 1, "automatically committing \"%s\"=\"%s\"\n", 
              param->key, param->negotiated);
        
        c = param->negotiated[19];
        param->negotiated[19] = '\0';
        TRACE_CLEAN(TRACE_PARAM, 0, "* %25s:%20s *\n", param->key, 
                    param->negotiated); 
        param->negotiated[19] = c;
        
        if (param->reset) {
            TRACE(TRACE_PARAM, 1, "deleting value list for \"%s\"\n", 
                  param->key);
            if (param_val_delete_all(head, param->key)!=0) {
                TRACE_ERROR("param_val_delete_all() failed\n");
                PTP_ERROR;
            }
            param->reset = 0;
        }
        if (param->value_l) {
            if (param->type == ISCSI_PARAM_TYPE_DECLARE_MULTI) {
                for (item_ptr=param->value_l; item_ptr->next!=NULL; 
                     item_ptr=item_ptr->next) {
                    if (strlen(item_ptr->value)==0) break;
                }
                if (item_ptr==NULL) {
                    if ((item_ptr->next=
                         iscsi_malloc_atomic(sizeof(ISCSI_PARAMETER_VALUE_T)))
                        ==NULL) {
                        TRACE_ERROR("iscsi_malloc_atomic() failed\n");
                        PTP_ERROR;
                    }
                    item_ptr = item_ptr->next;
                    item_ptr->next = NULL;
                }
            } else {
                item_ptr = param->value_l; 
            } 
        } else {
            TRACE(TRACE_PARAM, 1, "allocating value ptr\n");
            if ((param->value_l=
                 iscsi_malloc_atomic(sizeof(ISCSI_PARAMETER_VALUE_T)))==NULL) {
                TRACE_ERROR("iscsi_malloc_atomic() failed\n");
                PTP_ERROR;
            }
            item_ptr = param->value_l; 
            item_ptr->next = NULL;
        }
        strcpy(item_ptr->value, param->negotiated);
    next: continue;
    }
#if 0
    if (!outgoing) {
        TRACE(TRACE_PARAM, 0, "===BEGIN %i BYTE ANSWER===\n", *text_len_out);
        param_text_print(text_out, *text_len_out);
        TRACE(TRACE_PARAM, 0, "===END %i BYTE ANSWER===\n", *text_len_out);
    }
#endif
    TRACE_CLEAN(TRACE_PARAM, 0, "****************************************"
                "**********\n");  
    
    PTP_CLEANUP;
    return 0;
}

void set_session_parameters( ISCSI_PARAMETER_T *head, 
                             ISCSI_SESS_PARAM_T *sess_params ) {
    // These parameters are standard and assuming that they are always
    // present in the list (head).
    memset( sess_params, 0, sizeof(ISCSI_SESS_PARAM_T) );
    sess_params->max_burst_length = param_atoi(head, "MaxBurstLength");
    sess_params->first_burst_length = param_atoi(head, "FirstBurstLength");
    sess_params->max_data_seg_length = 
        param_atoi(head, "MaxRecvDataSegmentLength");
    if (!param_equiv(head, "InitialR2T", "Yes")) {
        sess_params->initial_r2t = 1;
    } else {
        sess_params->initial_r2t = 0;
    }
    if (!param_equiv(head, "ImmediateData", "Yes")) {
        sess_params->immediate_data = 1;
    } else {
        sess_params->immediate_data = 0;
    }
}
