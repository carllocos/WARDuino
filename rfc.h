#ifndef RFC_H
#define RFC_H

#include "WARDuino.h"


class RFC {
    private:
        // short unsigned serializationSize;
        struct SerializeData {
            const unsigned char* raw;
            uint32_t size;
        };

        struct SerializeData * serializeRFC();
        struct SerializeData * serializeRFCallee();
        void deserializeRFCResult(void);
    public:
        const uint32_t fid;
        StackValue *args;
        const Type* type;
        StackValue *result;
        bool succes;
        char* exceptionMsg;
        uint16_t excpMsgSize;

        RFC(uint32_t t_fid, Type * t_type, StackValue * t_args = nullptr);
        void call(StackValue *args);
        void returnResult();

        //Client side
        static RFC* registerRFC(uint32_t t_fid, Type *t_type);
        static void unregisterRFC(uint32_t fid);
        static bool isRFC(uint32_t fid);
        static RFC* getRFC(uint32_t fid);
        static void clearRFCs(void);

        //Server side
        static RFC* registerRFCallee(uint32_t t_fid, Type *t_type, StackValue *t_args);
        static bool hasRFCallee(void);
        static RFC* currentCallee(void);
        static void removeRFCallee(void);
};


#endif
