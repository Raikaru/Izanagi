/* 0047b0c0 mdlReadAuxChunks: the retail RMD chunk reader unchanged, plus the [resource-model] glTF
 * override seam (include/p4pc/mod_models.h, docs/modding/models.md "Runtime glTF models"). With no
 * override table the only differences are one load of the table's active flag and a null test per
 * animation chunk. For a model whose file
 * has an override, the texture dictionary and clump are read inline and the clump's atomics are
 * replaced by the cooked glTF's (src/platform/models/p4model_build.c) before any later chunk binds to them;
 * a slot animation the override carries (schema=3) is read in place of the file's. */
typedef struct P4ModelOverride P4ModelOverride;
extern int p4_model_overrides_active(void);
extern const P4ModelOverride *p4_model_override_for_model(const void *model);
extern int p4_model_override_apply(const P4ModelOverride *override, void *clump);
extern void *p4_model_override_animation(const P4ModelOverride *override, void *clump, u32 layer, u32 slot);

s32 mdlReadAuxChunks(u8 *model)
{


    struct MdlEffectChunk {
        u16 first, last;
        u32 length, skip, flags, unknown10;
    } effect;
    struct RwChunkHeaderInfo {
        u32 type, length, version, buildNum;
        s32 isComplex;
    } chunk;
    struct RwMemory {
        u8 *start;
        u32 length;
    } memory;
    s32 metadata;
    u16 capacity;
    u16 sourceIndex;
    u16 matrixCount;
    MdlLoaderState *state;
    MdlDispatchAnimTable *layerResource;
    s32 index;
    Model *modelObject = (Model *)model;
    const P4ModelOverride *p4Override;
    extern void *func_003df3c0(void *stream, struct RwChunkHeaderInfo *header);
    extern u32 func_003e2910(void *stream, void *buffer, u32 length);
    extern void *func_003e2ce0(void *stream, u32 length);
    extern void *func_003e2f60(s32 type, s32 access, const void *memory);
    extern void *func_003c0f20(void *stream);
    extern void *func_003d53c0(void *stream);
    extern void *func_003d6350(void *schema, void *stream);
    extern struct RwTexDictionary *func_003dc370(void *stream);
    extern struct RwTexDictionary *func_003e6a90(void *stream);
    extern s32 func_003ef1b0(void *dictionary);
    /* Pointer parameters keep D_0070B610 an address constant per call; the u32 definition in
     * rtanim.c would get hoisted. */
    extern u32 func_003d60e0(void *object, void *value);
    extern u8 *filerEnqueueRequest(s32 kind, const char *name, const char *path, s32 flags, s32 source, s32 buffer, s32 byteCount, const char *cacheName, const void *resultKind, s32 memoryKind);
    extern u32 mdlEffectListCreate(void);
    extern u32 *mdlEffectListAppendBlock(u32 **list, s32 source, u32 size, u16 matrixIndex, u32 flags);
    extern u32 *mdlEffectListAppendNode(u32 *list, u32 *sourceNode, u16 matrixIndex);
    extern u32 *mdlEffectListWrapCopy(s32 source, s32 size);
    extern s32 *mdlMatAnimCreateList(void);
    extern s32 mdlMatAnimLoadChunk(void *stream, u8 **list, u32 chunkTag, void *clump);
    extern u8 D_0070B610[];

#define LOAD_LAYER() ((MdlCloneLayer *)(model + 0xec + (u32)state->layer * 0xa4))
#define LOAD_MATERIALS() (*(MdlLoaderMaterialTable **)&modelObject->materialResource)
#define LOAD_UVS() (*(MdlLoaderUvTable **)&modelObject->animControl.resource)
#define LOAD_MEMORY() do { \
    u32 position = ((MdlMemoryStream *)(u8 *)state->stream)->position; \
    memory.start = state->memory + position; \
    memory.length = state->memoryLength - position; \
} while (0)

    state = *(MdlLoaderState * *)&((Model *)model)->auxiliary;
    /* A [resource-model] override of this model's file: its texture dictionary and clump are read
     * inline (as with flag 0x4000) and the clump's atomics are replaced right after it is read, before
     * any later chunk binds to its materials. NULL with no declaration: one load, then the retail path. */
    p4Override = p4_model_overrides_active() ? p4_model_override_for_model(model) : 0;
    capacity = 32;
    {
        u16 c;

        for (c = 0; c < 2; c++) {
            u16 *destination = &state->capacities[c];
            *destination = 32;
        }
    }
    while (func_003df3c0(state->stream, &chunk) != 0) {
        if (chunk.type == 0) {
            break;
        }
        switch (chunk.type) {

        case 0x16:
        if ((((Model *)model)->flags & 0x4000) != 0 || p4Override != 0) {
            struct RwTexDictionary *dictionary = func_003e6a90(state->stream);
            func_003ef260(dictionary, K_Clump_AppendTextureName, &state->textureList);
            func_003ef1b0(dictionary);
        } else {
            LOAD_MEMORY();
            state->textureRequest = filerEnqueueRequest(8, 0, 0, 0,
                (s32)func_003e2f60(3, 1, &memory), 0, 0, 0, 0, 0);
            func_003e2ce0(state->stream, chunk.length);
        }
        continue;

        case 0x23:
        {
            struct RwTexDictionary *dictionary = func_003dc370(state->stream);
            func_003ef260(dictionary, K_Clump_AppendTextureName, &state->textureList);
            func_003ef1b0(dictionary);
        }
        continue;

        case 0x1b:
        {
            void *animation;
            if (LOAD_LAYER()->resource == 0) {
                MdlDispatchAnimTable *table = (MdlDispatchAnimTable *)mdlDispatchAnimTableCreate(capacity);
                LOAD_LAYER()->resource = table;
            }
            /* A layer's base animation always stays the file's. */
            animation = 0;
            if (p4Override != 0 && (state->baseAnimation == 0 || LOAD_LAYER()->resource->unknown != 0)) {
                animation = p4_model_override_animation(p4Override, modelObject->clump, state->layer, state->slot);
            }
            if (animation != 0) {
                func_003e2ce0(state->stream, chunk.length);
            } else {
                animation = func_003d53c0(state->stream);
            }
            if (state->baseAnimation == 0 || LOAD_LAYER()->resource->unknown != 0) {
                layerResource = LOAD_LAYER()->resource;
                ((MdlDispatchAnimEntry *)((u8 *)layerResource->entries + (state->slot) * sizeof(MdlDispatchAnimEntry)))->animation = animation;
            } else {
                LOAD_LAYER()->resource->unknown = (u32)animation;
            }
        }
        continue;

        case 0x2b:
        if ((((Model *)model)->flags & 0x4000) != 0) {
            void *dictionary;
            MdlLoaderUvTable *table = LOAD_UVS();
            if (table == 0) {
                u16 count = capacity;
                s32 size = (u32)count * sizeof(MdlLoaderEntry) + sizeof(MdlLoaderUvTable);
                chkmemAllocSiteTag(D_00713138, 0x908);
                table = ((void *(*)(int, int))DAT_008873e8[0])(size, 0x40000);
                memset(table, 0, size);
                table->entries = (MdlLoaderEntry *)(table + 1);
                table->count = count;
                table->references = 1;
                LOAD_UVS() = table;
            }
            dictionary = func_003d6350(D_0070B610, state->stream);
            if (((Model *)model)->clump == 0) {
                LOAD_UVS()->base = dictionary;
                func_003d60e0(D_0070B610, dictionary);
            } else {
                LOAD_UVS()->entries[state->slot].value = dictionary;
            }
        } else {
            if (state->clumpStream == 0 && state->clumpRequest == 0) {
                LOAD_MEMORY();
                state->uvRequest = filerEnqueueRequest(7, 0, 0, 0,
                    (s32)func_003e2f60(3, 1, &memory), 0, 0, 0, D_0070B610, 0);
            } else {
                if (state->uvRequests == 0) {
                    chkmemAllocSiteTag(D_00713138, 0x1834);
                    state->uvRequests = ((void *(*)(int, int))DAT_008873e8[0])((u32)capacity * 4, 0x40000);
                    memset(state->uvRequests, 0, (u32)capacity * 4);
                }
                LOAD_MEMORY();
                {
                    u8 *request = filerEnqueueRequest(7, 0, 0, 0,
                        (s32)func_003e2f60(3, 1, &memory), 0, 0, 0, D_0070B610, 0);
                    state->uvRequests[state->slot] = request;
                }
            }
            func_003e2ce0(state->stream, chunk.length);
        }
        continue;

        case 0x10:
        if ((((Model *)model)->flags & 0x4000) != 0 || p4Override != 0) {
            if (modelObject->clump == 0) {
                void *clump = func_003c0f20(state->stream);
                if (p4Override != 0 && clump != 0) {
                    p4_model_override_apply(p4Override, clump);
                }
                ((Model *)model)->clump = clump;
            }
        } else {
            if (((Model *)model)->clump == 0 && state->clumpStream == 0 && state->clumpRequest == 0) {
                LOAD_MEMORY();
                state->clumpStream = func_003e2f60(3, 1, &memory);
            }
            func_003e2ce0(state->stream, chunk.length);
        }
        continue;

        case 0x1e:
            continue;
        case 0xf0f00001:
        if (LOAD_LAYER()->resource == 0) {
            MdlDispatchAnimTable *table = (MdlDispatchAnimTable *)mdlDispatchAnimTableCreate(capacity);
            LOAD_LAYER()->resource = table;
        }
        (LOAD_LAYER()->resource)->entries[state->slot].animation = (D_00922BC0_abs);
        func_003e2ce0(state->stream, chunk.length);
        continue;

        case 0xf0f00005:
        if (LOAD_LAYER()->resource == 0) {
            MdlDispatchAnimTable *table = (MdlDispatchAnimTable *)mdlDispatchAnimTableCreate(capacity);
            LOAD_LAYER()->resource = table;
        }
        layerResource = LOAD_LAYER()->resource;
        func_003e2910(state->stream, &layerResource->entries[state->slot].matrix, chunk.length);
        continue;

        case 0xf0f00004:
        state->slot++;
        func_003e2ce0(state->stream, chunk.length);
        continue;

        case 0xf0f00002:
        state->layer++;
        state->slot = 0;
        func_003e2ce0(state->stream, chunk.length);
        continue;

        case 0xf0f00003:
        if ((((Model *)model)->flags & 0x4000) != 0) {
            MdlDispatchAnimTable *animations;
            MdlCloneAttachmentTable *attachments;
            func_003e2910(state->stream, &sourceIndex, chunk.length);
            {
                u16 src = sourceIndex;
                u16 dst = state->slot;

                animations = LOAD_LAYER()->resource;
                {
                    MdlDispatchAnimEntry *entries = animations->entries;

                    entries[dst].matrix = entries[src].matrix;
                }
                if (*(u32 *)&animations->entries[src].animation != 0) {
                    *(u32 *)&animations->entries[dst].animation = *(u32 *)&animations->entries[src].animation;
                }
                animations->entries[dst].unknown44 |= 1;
            }
            {
                MdlLoaderMaterialTable *materials = LOAD_MATERIALS();

                if (materials != 0 && materials->count > sourceIndex) {
                    void *value = ((MdlLoaderEntry *)*(u8 **)materials)[sourceIndex].value;
                    if (value != 0) {
                        u16 dst = state->slot;

                        materials->entries[dst].value = value;
                        *(u8 *)&materials->entries[dst].flags |= 1;
                    }
                }
            }
            {
                MdlLoaderUvTable *uvs = LOAD_UVS();

                if (uvs != 0 && sourceIndex < uvs->count) {
                    void *value = ((MdlLoaderEntry *)*(u8 **)uvs)[sourceIndex].value;
                    if (value != 0) {
                        u16 dst = state->slot;

                        uvs->entries[dst].value = value;
                        *(u8 *)&uvs->entries[dst].flags |= 1;
                    }
                }
            }
            attachments = LOAD_LAYER()->attachments;
            if (attachments != 0) {
                u16 source = sourceIndex;
                u16 destination = state->slot;
                if (attachments->primary[source] != 0) {
                    void *copy = mdlEffectListCopy(attachments->primary[source]);
                    attachments->primary[destination] = copy;
                }
                if (attachments->secondary[source] != 0) {
                    void *copy = mdlEffectListWrapShared(attachments->secondary[source]);
                    attachments->secondary[destination] = copy;
                }
            }
        } else {
            func_003e2910(state->stream, &sourceIndex, chunk.length);
            if ((((MdlLoaderState *)((u8 *)state + state->layer * 4))->cloneSlots[0]) == 0) {
                u16 slot;
                chkmemAllocSiteTag(D_00713138, 0x1896);
                (((MdlLoaderState *)((u8 *)state + state->layer * 4))->cloneSlots[0]) = ((void *(*)(int, int))DAT_008873e8[0])((u32)capacity * 2, 0x40000);
                for (slot = 0; slot < capacity; slot++) {
                    (((MdlLoaderState *)((u8 *)state + state->layer * 4))->cloneSlots[0])[slot] = -1;
                }
            }
            (((MdlLoaderState *)((u8 *)state + state->layer * 4))->cloneSlots[0])[state->slot] = sourceIndex;
        }
        continue;

        case 0xf0f00080:
        case 0xf0f00081:
        case 0xf0f00083:
        case 0xf0f00082:
        if ((((Model *)model)->flags & 0x4000) != 0) {
            MdlLoaderMaterialTable *table = LOAD_MATERIALS();
            if (table == 0) {
                u16 count = capacity;
                s32 size = (u32)count * sizeof(MdlLoaderEntry) + sizeof(MdlLoaderMaterialTable);
                chkmemAllocSiteTag(D_00713138, 0x850);
                table = ((void *(*)(int, int))DAT_008873e8[0])(size, 0x40000);
                memset(table, 0, size);
                table->entries = (MdlLoaderEntry *)(table + 1);
                table->count = count;
                table->references = 1;
                LOAD_MATERIALS() = table;
            }
            if (LOAD_MATERIALS()->entries[state->slot].value == 0) {
                s32 *material = mdlMatAnimCreateList();
                LOAD_MATERIALS()->entries[state->slot].value = material;
            }
            mdlMatAnimLoadChunk(state->stream, LOAD_MATERIALS()->entries[state->slot].value,
                chunk.type, ((Model *)model)->clump);
        } else {
            MdlLoaderMaterial *pending;
            chkmemAllocSiteTag(D_00713138, 0x18b7);
            pending = ((void *(*)(int, int))DAT_008873e8[0])(sizeof(*pending), 0x40000);
            LOAD_MEMORY();
            pending->kind = chunk.type;
            pending->slot = state->slot;
            pending->stream = func_003e2f60(3, 1, &memory);
            func_003e2ce0(state->stream, chunk.length);
            pending->next = state->materials;
            state->materials = pending;
        }
        continue;

        case 0xf0f00070:
        {
            void *properties;
            chkmemAllocSiteTag(D_00713138, 0x18c5);
            properties = ((void *(*)(int, int))DAT_008873e8[0])(0x2b8, 0x40000);
            func_003e2910(state->stream, properties, chunk.length);
            mdlBuildScaledSlotRecords(model, properties);
            DAT_008873ec[0](properties);
        }
        continue;

        case 0xf0f00006:
        {
            void *stream = state->stream;
            MdlMatrixEntry *entries;
            MdlMatrixTable *table;
            s32 slot;
            s32 size;
            func_003e2910(stream, &matrixCount, 2);
            size = (u32)matrixCount * sizeof(MdlMatrixEntry) + sizeof(MdlMatrixTable);
            chkmemAllocSiteTag(D_00713138, 0xc93);
            entries = ((void *(*)(int, int))DAT_008873e8[0])(size, 0x40000);
            table = (MdlMatrixTable *)(entries + matrixCount);
            table->count = matrixCount;
            table->unknown = 1;
            table->entries = entries;
            for (slot = 0; (u16)slot < matrixCount; slot = (slot + 1) & 0xffff) {
                u32 offset = (slot & 0xffff) * sizeof(MdlMatrixEntry);
                func_003e2910(stream, (u8 *)table->entries + offset + 0x40, 4);
                func_003e2910(stream, (u8 *)table->entries + offset + 0x44, 4);
                func_003e2910(stream, (u8 *)table->entries + offset, sizeof(RwMatrix));
            }
            *(MdlMatrixTable * *)&((Model *)model)->matrixTable = table;
        }
        continue;

        case 0xf0f000f0:
        func_003e2910(state->stream, &capacity, chunk.length);
        (((MdlLoaderState *)((u8 *)state + state->layer * 2))->capacities[0]) = capacity;
        continue;

        case 0xf0f000e0:
        {
            u32 **head;
            u8 *payload;
            u32 *node;
            u16 slot;
            func_003e2910(state->stream, &effect, sizeof(effect));
            func_003e2ce0(state->stream, effect.skip);
            payload = state->memory + ((MdlMemoryStream *)(u8 *)state->stream)->position;
            if (((Model *)model)->clump == 0 && state->clumpStream == 0) {
                if (modelObject->effects == 0) {
                    *(u32 *)&((Model *)model)->effects = mdlEffectListCreate();
                }
                head = *(u32 ** *)&((Model *)model)->effects;
            } else {
                if (LOAD_LAYER()->attachments == 0) {
                    MdlCloneAttachmentTable *table = mdlCloneAttachmentStorage(capacity);
                    LOAD_LAYER()->attachments = table;
                }
                if (LOAD_LAYER()->attachments->primary[state->slot] == 0) {
                    void *wrapper = (void *)mdlEffectListCreate();
                    LOAD_LAYER()->attachments->primary[state->slot] = wrapper;
                }
                head = LOAD_LAYER()->attachments->primary[state->slot];
            }
            node = mdlEffectListAppendBlock(head, (s32)payload, (s32)effect.length, effect.first, effect.flags);
            for (slot = effect.first + 1; slot < effect.last + 1; slot++) {
                mdlEffectListAppendNode((u32 *)head, node, slot);
            }
            func_003e2ce0(state->stream, effect.length);
        }
        continue;

        case 0xf0f000e1:
        {
            MdlCloneAttachmentTable *table;
            s32 size;
            u32 count;
            u8 *payload = state->memory + ((MdlMemoryStream *)(u8 *)state->stream)->position;
            if (LOAD_LAYER()->attachments == 0) {
                count = capacity;
                size = sizeof(MdlCloneAttachmentTable);
                size += count * sizeof(void*);
                size += count * sizeof(void*);
                chkmemAllocSiteTag(D_00713138, 0x1d6);
                table = ((void* (*)(int, int))DAT_008873e8[0])(size, 0x40000);
                memset(table, 0, size);
                table->count.word = (u16)count;
                table->primary = (void**)(table + 1);
                table->secondary = table->primary + (u16)count;
                LOAD_LAYER()->attachments = table;
            }
            {
                u32 *wrapper = mdlEffectListWrapCopy((s32)payload, chunk.length);
                LOAD_LAYER()->attachments->secondary[state->slot] = wrapper;
            }
            func_003e2ce0(state->stream, chunk.length);
        }
        continue;

        case 0xf0f000d0:
        {
            u8 *group;
            u16 slot;
            chkmemAllocSiteTag(D_00713138, 0xfa);
            group = ((void *(*)(int, int))DAT_008873e8[0])(0x4c, 0x40000);
            memset(group, 0, 0x4c);
            ((MdlDirectionalBlend *)group)->weight = 1.0f;
            ((MdlDirectionalBlend *)group)->progress = 1.0f;
            layerResource = LOAD_LAYER()->resource;
            ((MdlDispatchAnimEntry *)((u8 *)layerResource->entries + (state->slot) * sizeof(MdlDispatchAnimEntry)))->blendControl = group;
            layerResource = LOAD_LAYER()->resource;
            func_003e2910(state->stream, ((MdlDispatchAnimEntry *)((u8 *)layerResource->entries + (state->slot) * sizeof(MdlDispatchAnimEntry)))->blendControl + 0x3c,
                chunk.length);
            for (slot = 0; slot < 4; slot++) {
                func_003df3c0(state->stream, &chunk);
                {
                    void *animation = func_003d53c0(state->stream);
                    layerResource = LOAD_LAYER()->resource;
                    ((void **)((MdlDispatchAnimEntry *)((u8 *)layerResource->entries + (state->slot) * sizeof(MdlDispatchAnimEntry)))->blendControl)[slot] = animation;
                }
            }
        }
        continue;

        case 0xf0f00007:
        case 0xf0f00008:
        func_003e2910(state->stream, &metadata, chunk.length);
        layerResource = LOAD_LAYER()->resource;
        if (((MdlDispatchAnimEntry *)((u8 *)layerResource->entries + (state->slot) * sizeof(MdlDispatchAnimEntry)))->startFrame == 0) {
            s32 *values;
            chkmemAllocSiteTag(D_00713138, 0x125);
            values = ((void *(*)(int, int))DAT_008873e8[0])(8, 0x40000);
            memset(values, 0, 8);
            layerResource = LOAD_LAYER()->resource;
            ((MdlDispatchAnimEntry *)((u8 *)layerResource->entries + (state->slot) * sizeof(MdlDispatchAnimEntry)))->startFrame = values;
        }
        if (chunk.type == 0xf0f00007) {
            (LOAD_LAYER()->resource)->entries[state->slot].startFrame[0] = (metadata);
        } else {
            (LOAD_LAYER()->resource)->entries[state->slot].startFrame[1] = (metadata);
        }
        continue;

        case 0xf0f00009:
        state->baseAnimation = 1;
        func_003e2ce0(state->stream, chunk.length);
        continue;

        default:
        func_003e2ce0(state->stream, chunk.length);
        }
    }
#undef LOAD_MEMORY
#undef LOAD_UVS
#undef LOAD_MATERIALS
#undef LOAD_LAYER
    return 1;
}
