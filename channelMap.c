#include "channelMap.h"
#include <string.h>
struct ChannelMap *channelMapInit(int size)
{
    struct ChannelMap *map = (struct ChannelMap *)malloc(sizeof(struct ChannelMap));
    if(map==NULL)
    {
        perror("malloc ChannelMap failed");  // 打印错误原因（如“Out of memory”）
        return NULL;
    }
    map->size = size;
    map->list = (struct Channel **)malloc(size * sizeof(struct Channel *));
    if (map->list == NULL)  // 关键：检查指针数组分配结果
    {
        perror("malloc Channel* list failed");
        free(map);  // 避免内存泄漏：释放已分配的 map 结构体
        return NULL;
    }
    memset(map->list, 0, size * sizeof(struct Channel *));
    return map;
}
void ChannelMapClear(struct ChannelMap *map)
{
    if (map != NULL)
    {
        for (int i = 0; i < map->size; i++)
        {
            if (map->list[i] != NULL)
                free(map->list[i]);
        }
        free(map->list);
        map->list = NULL;
    }
    map->size = 0;
}
bool makeMapRoom(struct ChannelMap *map, int newSize, int unitSize)
{
    if (map->size < newSize)
    {
        int curSize = map->size;
        // 容量扩大为原来一倍
        while (curSize < newSize)
        {
            curSize *= 2;
        }
        // 扩容realloc
        struct Channel **temp= realloc(map->list, curSize * unitSize);
        if (temp == NULL)
        {
            return false;
        }
        map->list = temp;
        memset(&map->list[map->size],0,(curSize-map->size)*unitSize);
        map->size=newSize;
    }
    return true;
}