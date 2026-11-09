#include "assets.h"

Assets gAssets = {0};

static Texture2D LoadSprite(const char *path)
{
    Texture2D t = LoadTexture(path);

    if (t.id == 0)
    {
        TraceLog(LOG_WARNING, "Textura nao carregou: %s (usando formas no lugar)", path);
        return t;
    }

    // POINT = pixel art nítida. Se sua arte for HD/suave, troque por TEXTURE_FILTER_BILINEAR.
    SetTextureFilter(t, TEXTURE_FILTER_POINT);
    return t;
}

void LoadAssets(void)
{
    gAssets.player = LoadSprite("assets/enemies/player.png");
    gAssets.memBlock = LoadSprite("assets/enemies/mem_block.png");

    gAssets.enemy[0] = LoadSprite("assets/enemies/virus_kill.png");
    gAssets.enemy[1] = LoadSprite("assets/enemies/virus_stun.png");
    gAssets.enemy[2] = LoadSprite("assets/enemies/virus_reverse.png");
}

void UnloadAssets(void)
{
    // UnloadTexture ignora texturas com id 0? Não confie: confira antes.
    if (gAssets.player.id != 0)
        UnloadTexture(gAssets.player);
    if (gAssets.memBlock.id != 0)
        UnloadTexture(gAssets.memBlock);

    for (int i = 0; i < 3; i++)
        if (gAssets.enemy[i].id != 0)
            UnloadTexture(gAssets.enemy[i]);

    gAssets = (Assets){0};
}
