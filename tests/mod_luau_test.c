/* Real Luau VM/bindings + production cooperative kernel, synthetic data only. */
#include "lua_runtime.h"
#include "native_bindings.h"
#include "p4pc/mod_script.h"
#include "guest_test.h"
#include "p4pc/image.h"
#include "p4pc/mod_events.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/os.h"
#include "p4pc/platform.h"
#include "p4pc/tables.h"
#include "p4pc/ps2_libc.h"
#include "vfs.h"
#include "../src/platform/memcard/mod_save_internal.h"
#ifdef P4_MOD_NATIVE_RECOVERY
#include "failure_native.h"
#include "../src/platform/config/p4cfg.h"
#include "p4pc/mods.h"
#include "p4pc/package_memory.h"
#include "p4pc/mod_bindings.h"
#include "p4pc/package_boot.h"
#endif
#include <fenv.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint32_t mod_test_unsigned(uint32_t value);
int16_t mod_test_narrow(int8_t value, uint16_t add);
float mod_test_mixed(int value, float real, unsigned char *byte, double wide);
uint32_t *mod_test_pointer(uint32_t *value);
int mod_test_recursive(int depth);
void mod_test_void(uint32_t *value);
const p4pc_native_t p4pc_native_table[] = {
    {0x100100, (void *)mod_test_unsigned, "mod_test_unsigned", NULL},
    {0x100120, (void *)mod_test_narrow, "mod_test_narrow", NULL},
    {0x100140, (void *)mod_test_mixed, "mod_test_mixed", NULL},
    {0x100160, (void *)mod_test_pointer, "mod_test_pointer", NULL},
    {0x100180, (void *)mod_test_recursive, "mod_test_recursive", NULL},
    {0x1001a0, (void *)mod_test_void, "mod_test_void", NULL}
};
const size_t p4pc_native_count = sizeof p4pc_native_table / sizeof p4pc_native_table[0];
const p4pc_patch_t p4pc_patch_table[] = {{0, NULL, NULL}};
const size_t p4pc_patch_count = 0;
const p4pc_hostfn_t p4pc_hostfn_table[] = {{NULL, NULL}};
const size_t p4pc_hostfn_count = 0;
/* This fixture never loads an ELF or initializes the EE heap. Their production
 * evidence getters have separate loader/real-allocator witnesses. The actual
 * kernel supplies stack exclusion, context keys and scheduling here. */
const p4pc_image_guest_range_t *p4pc_image_guest_ranges(size_t *count) { *count = 0; return NULL; }
int p4pc_heap_guest_span(uint32_t *begin, uint32_t *end) { (void)begin; (void)end; return -1; }
int p4pc_heap_guest_host_overlap(uint32_t address, size_t size) { (void)address; (void)size; return 0; }

int32_t func_004215e0(void *parameter);
int32_t func_00421600(int32_t id, uint32_t arg);
int32_t func_00421670(int32_t id, int32_t priority);
int32_t func_00421690(int32_t priority);
int32_t func_004216d0(void);
static int failures;
static unsigned canonical_copies, watched_invalidations;
static const void *watched_scratch;
static unsigned char watched_value;
static void observe_bytes(void *destination, const void *source, uint32_t count, int after)
{
    if (after) return;
    if (source) ++canonical_copies;
    if (!source && destination == watched_scratch && count == 128) {
        /* Reading here must happen before the real guest stack arena unmaps. */
        watched_value = ((const volatile unsigned char *)destination)[40];
        ++watched_invalidations;
    }
}
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "mod_luau: FAIL line%d: %s\n", __LINE__, #x); ++failures; } } while (0)
static int event_teardown_mode;
static int event_teardown(const P4ModEvent *event, void *user)
{
    (void)event; (void)user;
    CHECK(!p4_mod_events_quiescent());
    if (event_teardown_mode == 1) p4_mod_events_clear();
    else p4_mod_events_remove_owner(P4_MOD_NATIVE_OWNER);
    return 0;
}
static void run(P4LuaMod *mod, const char *source, int success)
{
    int status = p4_lua_compile(mod, source, strlen(source), "@mod-luau-test");
    if (!status) status = p4_lua_execute(mod);
    if ((status == 0) != success) fprintf(stderr, "mod_luau unexpected status %d for: %s\n", status, source);
    CHECK((status == 0) == success);
}
static void saved_provenance(int expected)
{
    unsigned char image[4100]={0};
    image[57]=0x20;image[60]=1;memset(image+65,0xff,4);
    P4ModSaveImage raw={image,sizeof image,"native-vm-synthetic"};
    P4ModSaveCapture *capture=NULL;char error[1024]={0};
    int result=p4_mod_save_capture_image(&raw,&capture,error,sizeof error);
    if(result)fprintf(stderr,"synthetic VM provenance capture: %s\n",error);
    CHECK(!result&&((capture!=NULL)==expected));
    CHECK(!capture||!capture->witness);
    p4_mod_save_capture_free(capture);
}
static int nested_callback(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    lua_getglobal(L, "quota_inner");
    if (p4_lua_protected(mod, L, 0, 0, 0)) luaL_error(L, "nested callback budget failed");
    return 0;
}
static int synchronous_callback(lua_State *L)
{
    lua_getglobal(L, "action_body");
    lua_call(L, 0, 0);
    return 0;
}
static int empty_callback(lua_State *L)
{
    (void)L;
    return 0;
}
static int yield_body(P4ModCall *call, void *user)
{
    (void)call; (void)user;
    CHECK(func_00421690(60) == 0);
    return 0;
}
static uint32_t worker_results[2];
static uint64_t worker_keys[2];
static void worker(void *argument)
{
    unsigned which = (unsigned)(uintptr_t)argument;
    worker_keys[which] = p4pc_thread_context_key();
    worker_results[which] = mod_test_unsigned(which ? 20u : 10u);
}
static int32_t spawn(void)
{
    struct {
        int32_t status;
        uint32_t function, stack;
        int32_t stack_size;
        uint32_t gp;
        int32_t initial_priority, current_priority;
        uint32_t attributes, option;
    } parameters = {.function = P4_FN_H2G(worker), .initial_priority = 60, .current_priority = 60};
    return func_004215e0(&parameters);
}
static void coroutine_witness(P4LuaMod *mod)
{
    run(mod,
        "trace={}; replacement=p4.hook(0x100100,'II','replace',function(c)\n"
        " local value=c.args[1]; table.insert(trace,'enter'..value)\n"
        " if value==10 then saved_first=c else assert(not pcall(saved_first.next)) end\n"
        " local result=c.next(); assert(result==value+1)\n"
        " table.insert(trace,'exit'..value); c.result=result*2\n"
        "end)", 1);
    P4ModFunction *function = p4_mod_function_find(0x100100, "II");
    P4ModHook *observer = p4_mod_hook_add(P4_MOD_NATIVE_OWNER, function, P4_MOD_BEFORE, yield_body, NULL);
    CHECK(observer != NULL);
    int32_t main_id = func_004216d0();
    CHECK(func_00421670(main_id, 50) >= 0);
    int32_t first = spawn(), second = spawn();
    CHECK(first > 1 && second > first);
    CHECK(func_00421600(first, 0) == first);
    CHECK(func_00421600(second, 1) == second);
    CHECK(func_00421670(main_id, 70) == 50);
    CHECK(worker_results[0] == 22 && worker_results[1] == 42);
    run(mod, "assert(table.concat(trace,',')=='enter10,enter20,exit10,exit20'); assert(not pcall(saved_first.next))", 1);
    uint64_t old_key = worker_keys[0];
    CHECK(func_00421670(main_id, 50) == 70);
    CHECK(func_00421600(first, 0) == first);
    CHECK(func_00421670(main_id, 70) == 50);
    CHECK(worker_keys[0] != old_key && worker_results[0] == 22);
    CHECK(func_00421670(main_id, 50) == 70);
    CHECK(p4_mod_hook_remove(observer) == 0);
    run(mod, "replacement:remove(); replacement=nil; saved_first=nil", 1);
}

static unsigned native_initializations, native_destructions;
static void native_initialize(lua_State *L)
{
    ++native_initializations;
    lua_pushboolean(L, 1); lua_setfield(L, -2, "fixture_native");
}
static void native_before_destroy(P4LuaMod *mod)
{
    ++native_destructions;
    /* Real Lua state remains alive; no fake settings backend is installed. */
    lua_getglobal(mod->state, "p4");
    lua_rawgetfield(mod->state, -1, "fixture_native");
    CHECK(lua_toboolean(mod->state, -1));
    lua_pop(mod->state, 2);
}
static const P4LuaNativeBindings fixture_bindings = {native_initialize, native_before_destroy};
static void adopt_native_context(void)
{
    #ifdef P4_GUEST64
    for (size_t i = 0; i < p4pc_native_count; ++i)
        p4_fn_register(p4pc_native_table[i].addr, p4pc_native_table[i].fn);
    #endif
    /* Adopt the real boot context before any VM/native call can change its key. */
    CHECK(func_004216d0() == 1);
}
static int benchmark_requested;
static int benchmark_noop(P4ModCall *call,void *user)
{
    (void)call;(void)user;return 0;
}
static uint64_t benchmark_cpu(void)
{
    uint64_t value=0;
    CHECK(!p4_os_thread_cpu_ns(&value));
    return value;
}
static uint32_t benchmark_calls(unsigned count)
{
    uint32_t sum=0;
    for(unsigned i=0;i<count;++i) {
        /* Advance the real production frame counter, not a test budget bypass. */
        p4pc_vsync_wait();
        sum+=mod_test_unsigned(10);
    }
    CHECK(sum==11u*count);
    return sum;
}
static int hook_cost(P4LuaMod *mod,uint64_t setup_start)
{
    enum { BATCHES=10,CALLS=20000,WARM=1000 };
    const uint64_t budget_ns=10000;
    uint64_t quantum=0,baseline=0;
    CHECK(!p4_os_thread_cpu_resolution_ns(&quantum)&&quantum);
    printf("hook_cost synthetic=true reference_eligible=false pointer_bits=%u setup_cpu_ns=%llu counter_quantum_ns=%llu\n",
        (unsigned)(8*sizeof(void *)),(unsigned long long)(benchmark_cpu()-setup_start),
        (unsigned long long)quantum);
    static const char *const names[]={"unhooked","native","luau","luau-before-after"};
    for(unsigned mode=0;mode<4;++mode) {
        uint64_t configure_start=benchmark_cpu();
        P4ModHook *native=NULL;
        if(mode==1) {
            native=p4_mod_hook_add(P4_MOD_NATIVE_OWNER,p4_mod_function_find(0x100100,"II"),
                P4_MOD_BEFORE,benchmark_noop,NULL);
            CHECK(native!=NULL);
        } else if(mode>=2) {
            run(mod,"bench_before=p4.hook(0x100100,'II','before',function(c) end)",1);
            if(mode==3)run(mod,"bench_after=p4.hook(0x100100,'II','after',function(c) end)",1);
        }
        if(failures)break;
        uint64_t configure_ns=benchmark_cpu()-configure_start,warm_start=benchmark_cpu();
        uint32_t checksum=benchmark_calls(WARM);
        uint64_t warm_ns=benchmark_cpu()-warm_start,samples[BATCHES],total=0;
        for(unsigned batch=0;batch<BATCHES;++batch) {
            uint64_t start=benchmark_cpu();
            checksum+=benchmark_calls(CALLS);
            uint64_t end=benchmark_cpu();
            CHECK(end>=start);
            samples[batch]=end-start;total+=samples[batch];
        }
        printf("hook_cost case=%s calls=%u batches=%u calls_per_batch=%u warm_calls=%u configure_cpu_ns=%llu warm_cpu_ns=%llu checksum=%u samples_cpu_ns=",
            names[mode],BATCHES*CALLS,BATCHES,CALLS,WARM,
            (unsigned long long)configure_ns,(unsigned long long)warm_ns,checksum);
        for(unsigned i=0;i<BATCHES;++i)printf("%s%llu",i?",":"",(unsigned long long)samples[i]);
        for(unsigned i=1;i<BATCHES;++i) {
            uint64_t value=samples[i];unsigned j=i;
            while(j&&samples[j-1]>value){samples[j]=samples[j-1];--j;}
            samples[j]=value;
        }
        if(!mode)baseline=total;
        uint64_t added=total>baseline?total-baseline:0;
        unsigned callbacks=mode==3?2u:1u;
        /* Each independent interval can be off by one counter quantum. */
        uint64_t uncertainty=2u*BATCHES*quantum;
        printf(" total_cpu_ns=%llu median_ns_per_call=%llu p95_ns_per_call=%llu added_ns_per_callback=%llu uncertainty_ns_per_callback=%llu budget_ns_per_callback=%llu\n",
            (unsigned long long)total,(unsigned long long)((samples[(BATCHES-1)/2]+samples[BATCHES/2])/2/CALLS),
            (unsigned long long)(samples[BATCHES-1]/CALLS),
            (unsigned long long)(added/(BATCHES*CALLS*callbacks)),
            (unsigned long long)(uncertainty/(BATCHES*CALLS*callbacks)),
            (unsigned long long)budget_ns);
        if(mode>=2)CHECK(added<=budget_ns*BATCHES*CALLS*callbacks+uncertainty);
        if(native)CHECK(!p4_mod_hook_remove(native));
        if(mode>=2)run(mod,"bench_before:remove(); bench_before=nil",1);
        if(mode==3)run(mod,"bench_after:remove(); bench_after=nil",1);
    }
    return failures?1:0;
}
static int frame_cpu_witness(int argc,char **argv)
{
    (void)argc;(void)argv;
    CHECK(func_004216d0()==1);
    volatile uint32_t work=1;
    for(unsigned frame=0;frame<2101;++frame) {
        for(unsigned i=0;i<4096;++i)work=work*1664525u+1013904223u;
        p4pc_vsync_wait();
    }
    CHECK(!p4_packages_count());
    printf("frame_cpu_fixture synthetic=true reference_eligible=false checksum=%u result=%s\n",
        (unsigned)work,failures?"FAIL":"PASS");
    return failures?1:0;
}

static int witness(int argc, char **argv)
{
    if (argc != 2) return 2;
    uint64_t setup_start=benchmark_requested?benchmark_cpu():0;
    CHECK(p4_lua_bind_native(&fixture_bindings) == 0);
    adopt_native_context();
    char path[1200];
    p4_os_mkdir(argv[1]);
    int length = snprintf(path, sizeof path, "%s/DATA", argv[1]);
    if (length < 0 || (size_t)length >= sizeof path) return 2;
    p4_os_mkdir(path);
    length = snprintf(path, sizeof path, "%s/DATA/TEST.TXT", argv[1]);
    if (length < 0 || (size_t)length >= sizeof path) return 2;
    FILE *fixture = fopen(path, "wb");
    if (!fixture) return 2;
    int wrote = fputs("mod-resource", fixture);
    int closed = fclose(fixture);
    if (wrote < 0 || closed) return 2;
    char mod_root[1200];
    length = snprintf(mod_root, sizeof mod_root, "%s/mods", argv[1]);
    if (length < 0 || (size_t)length >= sizeof mod_root) return 2;
    p4_os_mkdir(mod_root);
    static const char *const ids[] = {"test.luau", "test.isolation"};
    for (unsigned i = 0; i < 2; ++i) {
        length = snprintf(path, sizeof path, "%s/%s", mod_root, ids[i]);
        if (length < 0 || (size_t)length >= sizeof path) return 2;
        p4_os_mkdir(path);
        for (unsigned source = 0; source < 2; ++source) {
            length = snprintf(path, sizeof path, "%s/%s/%s", mod_root, ids[i],
                source ? "main.luau" : "mod.ini");
            if (length < 0 || (size_t)length >= sizeof path) return 2;
            fixture = fopen(path, "wb");
            if (!fixture) return 2;
            wrote = source ? fputs("-- Native VM witness supplies source below.\n", fixture) :
                fprintf(fixture, "[mod]\nid=%s\nname=Native VM witness\nversion=1.0\napi=1.1\npriority=%u\norder=0\n", ids[i], i);
            closed = fclose(fixture);
            if (wrote < 0 || closed) return 2;
        }
    }
    if (p4_mods_stage(mod_root, "test.luau,test.isolation") || p4_package_registry_publish()) return 2;
    if (vfs_mount(NULL, argv[1])) return 2;
    const P4PackageProvider *provider = p4_packages_provider(0);
    const P4PackageProvider *other_provider = p4_packages_provider(1);
    CHECK(provider && other_provider && p4_mods_count() == 2);
    CHECK(p4pc_bytes_observer(observe_bytes) == 0);
    CHECK(p4_lua_create(NULL) == NULL);
    P4LuaMod *mod = p4_lua_create(provider);
    CHECK(mod != NULL);
    if (!mod) return 1;
    if(benchmark_requested) {
        int result=hook_cost(mod,setup_start);
        p4_lua_destroy(mod);
        p4_package_registry_retire();
        return result;
    }
    saved_provenance(0);
    run(mod,
        "local b=p4.mem.alloc(16); p4.mem.write('u32',b,7); p4.mem.write_string(b,16,'owned scratch'); b:release()\n"
        "assert(not pcall(p4.mem.write,'u32',0x100000,1))\n"
        "local h=p4.hook(0x100100,'II','before',function(c) end); h:remove()\n"
        "local e=p4.events.on('frame_start',function(e) end); e:remove()\n",1);
    CHECK(!mod->semantic_used);
    saved_provenance(0); /* Compile/registration/owned scratch are not gameplay. */
    run(mod,"semantic_hook=p4.hook(0x100100,'II','before',function(c) end)",1);
    saved_provenance(0);
    CHECK(mod_test_unsigned(10)==11&&mod->semantic_used);
    saved_provenance(1); /* Actual script dispatch records its selected pin. */
    run(mod,"semantic_hook:remove(); semantic_hook=nil",1);
    run(mod,
        "assert(p4.language=='Luau' and p4.language_version=='0.744' and p4.api_version=='1.1')\n"
        "assert(p4.fixture_native and p4.ui==nil)\n"
        "for _,key in {'os','io','debug','package','require','load','loadstring','loadfile','dofile','vector','integer'} do assert(_G[key]==nil,key) end\n"
        "assert(type(pcall)=='function' and type(xpcall)=='function' and type(getfenv)=='function' and type(coroutine.resume)=='function')\n"
        "assert(string.dump==nil and p4.mem._pin==nil)\n"
        "scratch=p4.mem.alloc(128); a=scratch.address; assert(scratch.size==128)\n"
        "p4.mem.write('u32',a+1,0xfedcba98); assert(p4.mem.read('u32',a+1)==0xfedcba98)\n"
        "p4.mem.write('u64',a+8,p4.u64(0xffffffff,0xfffffffe)); local hi,lo=p4.split64(p4.mem.read('u64',a+8)); assert(hi==0xffffffff and lo==0xfffffffe)\n"
        "p4.mem.write('i64',a+8,p4.u64(0x80000000,0)); hi,lo=p4.split64(p4.mem.read('i64',a+8)); assert(hi==0x80000000 and lo==0)\n"
        "p4.mem.write('i8',a,-128); assert(p4.mem.read('i8',a)==-128)\n"
        "p4.mem.write('i16',a,-32768); assert(p4.mem.read('i16',a)==-32768)\n"
        "p4.mem.write('i32',a,-2147483648); assert(p4.mem.read('i32',a)==-2147483648)\n"
        "p4.mem.write('f32',a+16,1.25); assert(p4.mem.read('f32',a+16)==1.25)\n"
        "p4.mem.write('f64',a+24,-7.125); assert(p4.mem.read('f64',a+24)==-7.125)\n"
        "p4.mem.write_string(a+32,4,'abc'); assert(p4.mem.read_string(a+32,4)=='abc')\n"
        "layout=p4.mem.layout{name='Record',size=16,fields={value={offset=0,type='i32'},items={offset=4,type='u16',count=4},name={offset=12,type='string',capacity=4}}}\n"
        "view=layout:view(a+40); view.value=-17; view.items[3]=65535; view.name='new'\n"
        "assert(view.value==-17 and #view.items==4 and view.items.count==4 and view.items[3]==65535 and view.name=='new')\n"
        "assert(view.items[5]==nil and not pcall(function() rawset(view,'value',1) end))\n"
        "assert(p4.call(0x100120,'hbH',-7,12)==5 and p4.call(0x100160,'pp',a)==a)\n"
        "local fn=p4.fn(0x100100); assert(fn.address==0x100100 and p4.call(fn,'II',10)==11)\n"
        "functions=p4.mem.layout{name='Functions',size=4,fields={target={offset=0,type='fn'}}}:view(a+96)\n"
        "functions.target=fn; assert(functions.target.address==0x100100)\n"
        "assert(not pcall(function() p4.mem.write('u8',a+97,1) end)); functions.target=nil\n", 1);
    run(mod, "assert(buffer.tostring(p4.files.read('cdrom0:\\\\data\\\\test.txt;1'))=='mod-resource')", 1);
    run(mod, "for i=1,100 do assert(not pcall(p4.files.read,'DATA/TEST.TXT',3)) end; assert(buffer.len(p4.files.read('DATA/TEST.TXT'))==12)", 1);
    run(mod, "p4.files.read('../outside')", 0);
    run(mod, "p4.mem.read('u32',0x1fffffe)", 0);
    run(mod, "p4.mem.write('u8',a,256)", 0);
    run(mod, "p4.mem.write('i16',a,32768)", 0);
    run(mod, "p4.mem.write('u32',a,0/0)", 0);
    run(mod, "p4.mem.write('u64',a,9007199254740992)", 0);
    run(mod, "p4.mem.write('u64',a,buffer.create(7))", 0);
    run(mod, "p4.mem.read('u8',newproxy(true))", 0);
    run(mod, "p4.mem.write_string(a+32,4,'long')", 0);
    run(mod, "assert(p4.mem.read_string(a+32,4)=='abc')", 1);
    run(mod, "p4.mem.write('u32',a+32,0x41414141); p4.mem.read_string(a+32,4)", 0);
    run(mod, "layout:view(a+40,true).value=5", 0);
    run(mod, "view.items[5]=1", 0);
    run(mod, "p4.mem.layout{name='Bad',size=2,fields={bad={offset=1,type='u32'}}}", 0);
    run(mod, "p4.call(0x100120,'iii',-7,12)", 0);
    run(mod, "p4.call(0x100120,'hbH',-129,12)", 0);
    run(mod, "p4.call(0x100121,'hbH',-7,12)", 0);
    run(mod, "p4.call(0x100120,'hbH',-7)", 0);
    run(mod, "while true do end", 0);
    run(mod, "pcall(function() while true do end end)", 0);
    lua_pushcfunction(mod->state, nested_callback, "test_nested");
    lua_setglobal(mod->state, "test_nested");
    run(mod, "quota_inner=function() end; while true do test_nested() end", 0);
    run(mod, "quota_inner=function() for i=1,15000 do end end; while true do test_nested() end", 0);
    run(mod, "test_nested=nil; quota_inner=nil", 1);
    run(mod, "local x=string.rep('x',16*1024*1024)", 0);
    run(mod, "error({bad=1})", 0);
    const char binary[] = "\033Lua\0not-source";
    CHECK(p4_lua_compile(mod, binary, sizeof binary - 1, "binary-refused") == -1);
    run(mod, "action_body=function() p4.call(0x100100,'II',1) end", 1);
    CHECK(p4_lua_invoke(mod, synchronous_callback, NULL, 0, P4_LUA_SYNCHRONOUS) == -1);
    run(mod,
        "before=p4.hook(0x100100,'II','before',function(c) c.args[1]+=2 end)\n"
        "after=p4.hook(0x100100,'II','after',function(c) c.result*=2 end)\n"
        "replacement=p4.hook(0x100100,'II','replace',function(c) saved=c; c.next(); c.result+=5 end)\n"
        "assert(p4.call(0x100100,'II',10)==11)\n"
        "recursive_calls=0; recursive=p4.hook(0x100180,'ii','before',function(c) recursive_calls+=1 end)\n", 1);
    CHECK(mod_test_unsigned(10) == 36);
    CHECK(mod_test_recursive(3) == 3);
    run(mod, "assert(recursive_calls==4)", 1);
    run(mod, "saved.next()", 0);
    run(mod, "replacement:disable()", 1);
    CHECK(mod_test_unsigned(10) == 26);
    run(mod, "before:remove(); replacement:enable()", 1);
    CHECK(mod_test_unsigned(10) == 32);
    P4LuaMod *other = p4_lua_create(other_provider);
    CHECK(other != NULL);
    if (other) {
        run(other, "assert(scratch==nil and saved==nil and type(buffer.create)=='function')", 1);
        run(other, "p4.hook(0x100100,'II','replace',function(c) c.result=0 end)", 0);
        lua_getglobal(mod->state, "a");
        uint32_t address = lua_tounsigned(mod->state, -1);
        lua_pop(mod->state, 1);
        char source[160];
        snprintf(source, sizeof source, "p4.mem.read('u8',%u)", (unsigned)address);
        run(other, source, 0);
        p4_lua_destroy(other);
    }
    run(mod, "before:remove(); after:remove(); replacement:remove(); recursive:remove()", 1);
    coroutine_witness(mod);
    run(mod, "frames=0; ev=p4.events.on('frame_start',function(e) assert(e.name=='frame_start'); frames+=e.frame end)", 1);
    p4_mod_event_frame(42);
    run(mod, "assert(frames==42); ev:disable()", 1);
    CHECK(!(p4_mod_event_mask & P4_MOD_FRAME_START_BIT));
    p4_mod_event_frame(2);
    run(mod, "assert(frames==42); ev:enable()", 1);
    p4_mod_event_frame(3);
    run(mod, "assert(frames==45); ev:remove()", 1);
    run(mod, "local d,p,r=p4.input.action('confirm'); assert(not d and not p and not r)", 1);
    uint8_t packet[32] = {0};
    packet[2] = 0xff; packet[3] = 0xbf;
    packet[4] = 255; packet[5] = 128; packet[6] = 0; packet[7] = 192;
    p4_mod_input_sample(packet, 1);
    run(mod, "local d,p,r=p4.input.action('confirm'); assert(d and p and not r); local x,y,rx,ry=p4.input.sticks(); assert(x==-1 and y==0.5 and rx==127/128 and ry==0)", 1);
    p4_mod_input_sample(packet, 2);
    run(mod, "local d,p,r=p4.input.action('confirm'); assert(d and not p and not r)", 1);
    packet[3] = 0xff;
    p4_mod_input_sample(packet, 3);
    run(mod, "local d,p,r=p4.input.action('confirm'); assert(not d and not p and r)", 1);
    int rounding = fegetround();
    fesetround(FE_DOWNWARD);
    run(mod, "p4.mem.write('f32',a+16,1.000000059604644775390625); assert(p4.mem.read('u32',a+16)==0x3f800000)", 1);
    CHECK(fegetround() == FE_DOWNWARD);
    fesetround(rounding);
    run(mod, "scratch:release(); assert(view.value==-17); view.value=-18; assert(view.value==-18); scratch=nil", 1);
    lua_gc(mod->state, LUA_GCCOLLECT, 0);
    run(mod, "assert(view.value==-18)", 1);
    other = p4_lua_create(other_provider);
    CHECK(other != NULL);
    if (other) {
        run(other, "for i=1,128 do p4.hook(0x100100,'II','before',function(c) end):remove() end", 1);
        CHECK(other->subscriptions == 128);
        run(other, "p4.hook(0x100100,'II','before',function(c) end)", 0);
        /* Empty callbacks still consume frame fuel; a real flip renews it and
         * exhaustion remains sticky until a new host-side source invocation. */
        run(other, "", 1);
        int exhausted = 0;
        for (unsigned i = 0; i < 4096; ++i)
            exhausted |= p4_lua_invoke(other, empty_callback, NULL, 0, 0);
        CHECK(!exhausted);
        p4pc_vsync_wait();
        for (unsigned i = 0; i < 8192; ++i)
            exhausted |= p4_lua_invoke(other, empty_callback, NULL, 0, 0);
        CHECK(!exhausted);
        CHECK(p4_lua_invoke(other, empty_callback, NULL, 0, 0) != 0);
        p4pc_vsync_wait();
        CHECK(p4_lua_invoke(other, empty_callback, NULL, 0, 0) != 0);
        run(other, "assert(true)", 1);
        p4_lua_destroy(other);
    }
    lua_getglobal(mod->state, "a");
    watched_scratch = P4_G2H(lua_tounsigned(mod->state, -1));
    lua_pop(mod->state, 1);
    p4_lua_destroy(mod);
    CHECK(canonical_copies > 0 && watched_invalidations == 1 && watched_value == 0xee);
    CHECK(p4pc_bytes_observer(NULL) == 0);
    p4_mod_events_clear();
    p4_mod_hooks_clear();
    CHECK(mod_test_unsigned(10) == 11);
    p4_package_registry_retire();
    CHECK(p4_mods_count() == 0);
    CHECK(native_initializations > 0 && native_initializations == native_destructions);
    printf("mod_luau: %s (sandbox, quotas, LE-wide buffers, pinned layouts, known function words, hooks, real non-LIFO coroutines/restarts, events/input, FP restore)\n",
        failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
#ifdef P4_MOD_NATIVE_RECOVERY
static unsigned managed_entries, good_source_reads, bad_source_reads;
static void managed_read(const P4PackageFileOrigin *origin, const void *bytes,
                         size_t size, uint64_t offset, void *user)
{
    (void)bytes; (void)size; (void)offset; (void)user;
    if (strcmp(origin->relative, "main.luau")) return;
    if (!strcmp(origin->package->id, "mod.good")) ++good_source_reads;
    else if (!strcmp(origin->package->id, "mod.bad")) ++bad_source_reads;
    else CHECK(0);
}
static int managed_witness(int argc, char **argv)
{
    CHECK(argc == 3 && ++managed_entries == 1);
    if (!p4_mod_witness_enabled()) return 2;
    CHECK(p4pc_thread_context_key() == 0);
    p4cfg_load();
    p4_os_setenv("P4PC_MOD_DIR",argv[2],1);
    if(p4_mod_native_failure_init()||p4_mod_script_failure_init())return 2;
    const char *enabled = p4cfg_str("mods.enabled", "");
    printf("managed entry mod_witness=true reference_eligible=false enabled=%s\n", enabled);
    if (
        p4_package_reads_observe(managed_read, NULL) ||
        p4_mods_init() ||
        p4_package_registry_publish() || vfs_mount(NULL, NULL)) {
        fputs("managed loader fixture could not prepare\n", stderr);
        return 2;
    }
    p4_package_boot_admission_complete();
    int bad = 0;
    for (size_t i = 0; i < p4_mods_count(); ++i)
        bad |= !strcmp(p4_mods_info(i)->id, "mod.bad");
    int empty = !strcmp(argv[1], "--managed-empty");
    CHECK(good_source_reads == (empty ? 0u : 1u));
    CHECK(p4_mod_event_mask == 0); /* All selected chunks compiled; none executed. */
    int fd = vfs_open("DATA/TEST.TXT", 0);
    char bytes[5] = {0};
    CHECK(empty ? fd < 0 : fd >= 0);
    if (fd >= 0) {
        CHECK(vfs_read(fd, bytes, sizeof bytes - 1) == (bad ? 3 : 4));
        CHECK(!strcmp(bytes, bad ? "bad" : "good"));
        CHECK(vfs_close(fd) == 0);
    }
    printf("managed compile barrier good_reads=%u bad_reads=%u mods=%zu mask=%u raw=%s\n",
           good_source_reads, bad_source_reads, p4_mods_count(), (unsigned)p4_mod_event_mask, bytes);
    fflush(stdout);
    adopt_native_context(); /* Pass1 also works before the native boot task exists. */
    p4_mod_scripts_start(); /* Runtime failure must use the real native backend. */
    CHECK(p4_mod_event_mask == (empty ? 0u : P4_MOD_FRAME_START_BIT));
    if (!strcmp(argv[1], "--managed-callback")) p4_mod_event_frame(1);
    if (!strcmp(argv[1], "--managed-hook")) CHECK(mod_test_unsigned(10) == 11);
    CHECK(!bad); /* A bad runtime chunk must never fall through after its error. */
    p4_mod_scripts_stop(); /* Fixture has no queued work; callback stacks are idle. */
    CHECK(p4_mod_event_mask == 0);
    p4_package_registry_retire();
    CHECK(p4_package_reads_unobserve(managed_read, NULL) == 0);
    P4PackageMemoryUsage memory = p4_packages_memory_usage();
    CHECK(memory.resident == 0 && memory.reserved == 0);
    p4_package_boot_clean_exit(); /* Normal host-fixture completion, not a fake title frame. */
    printf("managed %s: %s\n", empty || !strcmp(argv[1], "--managed-preflight") ? "pre-code refusal" : "fresh-entry recovery",
           failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
#endif
int main(int argc, char **argv)
{
    #ifdef P4_MOD_NATIVE_RECOVERY
    if (p4_os_restart_init(argc, argv)) return 2;
    #endif
    if (argc == 2 && (!strcmp(argv[1], "--active-clear") || !strcmp(argv[1], "--active-owner-remove"))) {
        event_teardown_mode = !strcmp(argv[1], "--active-clear") ? 1 : 2;
        CHECK(p4_mod_events_quiescent());
        if (!p4_mod_event_subscribe(P4_MOD_NATIVE_OWNER, "frame_start", event_teardown, NULL)) return 2;
        p4_mod_event_frame(1);
        return failures ? 1 : 0;
    }
    p4_os_init();
    p4_os_setenv("P4PC_VIRTUAL_CLOCK", "1", 1);
    p4_os_setenv("P4PC_TURBO", "0", 1);
    if(argc==2&&!strcmp(argv[1],"--frame-cpu"))
        return p4t_on_guest_stack(frame_cpu_witness,argc,argv);
    if(argc==3&&!strcmp(argv[1],"--hook-cost")) {
        benchmark_requested=1;
        return p4t_on_guest_stack(witness,argc-1,argv+1);
    }
    #ifdef P4_MOD_NATIVE_RECOVERY
    if (argc == 3 && (!strcmp(argv[1], "--managed-preflight") || !strcmp(argv[1], "--managed-runtime") ||
                      !strcmp(argv[1], "--managed-callback") || !strcmp(argv[1], "--managed-hook") ||
                      !strcmp(argv[1], "--managed-empty")))
        return p4t_on_guest_stack(managed_witness, argc, argv);
    #endif
    return p4t_on_guest_stack(witness, argc, argv);
}
