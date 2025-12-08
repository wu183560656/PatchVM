#ifndef SDK_H_H_
#define SDK_H_H_

#define DEFAULT_VMCALL_PASSWORD_ECX 0x12345678
#define DEFAULT_VMCALL_PASSWORD_EDX 0x87654321

typedef struct
{
    unsigned int Command;
    unsigned int reserved;
} VMCALL_HEADER;

#define VMCALL_EXIST 0x80000000

#define VMCALL_CHANGE_PASSWORD 0x80000001
typedef struct
{
    unsigned int NewPassWordEcx;
    unsigned int NewPassWordEdx;
}CHANGE_PASSWORD_DATA;

#define VMCALL_SET_PAGE_EXECUTE_PFN 0x80000002
typedef struct
{
    unsigned long long Pfn;
    unsigned long long ExecutePfn;
}SET_PAGE_EXECUTE_PFN_DATA;
#define VMCALL_GET_PAGE_EXECUTE_PFN 0x80000003
typedef struct
{
    unsigned long long Pfn;
    unsigned long long ExecutePfn;
}GET_PAGE_EXECUTE_PFN_DATA;

#endif  // SDK_H_H_