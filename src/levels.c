#include "raylib.h"
#include "raymath.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "levels.h"

static Vector2 TileToPixel(int col, int row)
{
    return (Vector2){col * TILE_SIZE + TILE_SIZE / 2.0f,
                     row * TILE_SIZE + TILE_SIZE / 2.0f}; // centro do tile
}

// Bloco e slot com o mesmo valor têm a mesma cor (valor 1 a 10)
static Color ValueColor(int value)
{
    const Color palette[10] = {PURPLE, ORANGE, BLUE, DARKGREEN, BROWN,
                               SKYBLUE, VIOLET, DARKPURPLE, DARKBROWN, GRAY};
    return palette[(value - 1) % 10];
}

// A patrulha é reta (mesma linha ou mesma coluna). Confere se não atravessa parede.
static bool PatrolIsClear(const Level *lvl, int ac, int ar, int bc, int br)
{
    int dc = (bc > ac) - (bc < ac);
    int dr = (br > ar) - (br < ar);
    int c = ac, r = ar;

    while (true)
    {
        if (lvl->tileMap[r][c] == TILE_WALL)
            return false;
        if (c == bc && r == br)
            return true;
        c += dc;
        r += dr;
    }
}

// ---------------------------------------------------------------------------------------------
// FORMATO DO ARQUIVO DE FASE
//
//   @name Nome da fase          (opcional)
//   @time 90                    (opcional, segundos; padrão 60)
//   @enemy <id> <K|S|R> <vel>   (opcional; id de 0 a 9; vel em pixels/s)
//   ...18 linhas com 32 caracteres cada:
//
//   #  parede            .  chão             =  trilha de barramento
//   P  início do player
//   0-9  pontos A e B da patrulha do inimigo com esse id (cada dígito aparece 2 vezes)
//   a-j  bloco de memória (a = valor 1, b = valor 2, ...)
//   A-J  slot de memória  (A = valor 1, B = valor 2, ...)  -> aceita o bloco de mesma letra
// ---------------------------------------------------------------------------------------------
static bool ParseLevel(FILE *f, const char *path, Level *lvl)
{
    EnemyType eType[MAX_ENEMIES];
    float eSpeed[MAX_ENEMIES];
    int pCount[MAX_ENEMIES] = {0};
    int pCol[MAX_ENEMIES][2] = {{0}};
    int pRow[MAX_ENEMIES][2] = {{0}};

    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        eType[i] = ENEMY_KILL;
        eSpeed[i] = 80.0f;
    }

    bool hasPlayer = false;
    char line[256];
    int row = 0;
    int lineNo = 0;

    while (fgets(line, sizeof line, f))
    {
        lineNo++;
        line[strcspn(line, "\r\n")] = '\0'; // tira \n e \r (Windows)

        // Alguns editores do Windows gravam um BOM no começo do arquivo
        if (lineNo == 1 && (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF)
            memmove(line, line + 3, strlen(line + 3) + 1);

        // ---- METADADOS ----
        if (line[0] == '@')
        {
            if (strncmp(line, "@name ", 6) == 0)
            {
                snprintf(lvl->levelName, sizeof lvl->levelName, "%s", line + 6);
            }
            else if (strncmp(line, "@time ", 6) == 0)
            {
                if (sscanf(line + 6, "%f", &lvl->timeLimit) != 1 || lvl->timeLimit <= 0.0f)
                {
                    TraceLog(LOG_ERROR, "%s:%d: @time invalido (use: @time 90)", path, lineNo);
                    return false;
                }
            }
            else if (strncmp(line, "@enemy ", 7) == 0)
            {
                int id;
                char t;
                float spd;

                if (sscanf(line + 7, "%d %c %f", &id, &t, &spd) != 3 ||
                    id < 0 || id >= MAX_ENEMIES || spd <= 0.0f)
                {
                    TraceLog(LOG_ERROR, "%s:%d: @enemy invalido (use: @enemy <0-9> <K|S|R> <velocidade>)",
                             path, lineNo);
                    return false;
                }

                t = (char)toupper((unsigned char)t);

                if (t == 'K')
                    eType[id] = ENEMY_KILL;
                else if (t == 'S')
                    eType[id] = ENEMY_STUN;
                else if (t == 'R')
                    eType[id] = ENEMY_REVERSE;
                else
                {
                    TraceLog(LOG_ERROR, "%s:%d: tipo de inimigo '%c' invalido (use K, S ou R)",
                             path, lineNo, t);
                    return false;
                }

                eSpeed[id] = spd;
            }
            else
            {
                TraceLog(LOG_WARNING, "%s:%d: metadado desconhecido ignorado: %s", path, lineNo, line);
            }
            continue;
        }

        if (line[0] == '\0')
            continue; // ignora linhas vazias

        // ---- LINHAS DO MAPA ----
        if (row >= MAP_ROWS)
        {
            TraceLog(LOG_ERROR, "%s:%d: o mapa tem linhas demais (maximo %d)", path, lineNo, MAP_ROWS);
            return false;
        }

        if ((int)strlen(line) != MAP_COLS)
        {
            TraceLog(LOG_ERROR, "%s:%d: a linha do mapa tem %d colunas (esperado %d)",
                     path, lineNo, (int)strlen(line), MAP_COLS);
            return false;
        }

        for (int c = 0; c < MAP_COLS; c++)
        {
            char ch = line[c];
            lvl->tileMap[row][c] = TILE_FLOOR; // terreno por padrão

            if (ch == '#')
            {
                lvl->tileMap[row][c] = TILE_WALL;
            }
            else if (ch == '.')
            {
                // chão
            }
            else if (ch == '=')
            {
                lvl->tileMap[row][c] = TILE_BUS;
            }
            else if (ch == 'P')
            {
                if (hasPlayer)
                {
                    TraceLog(LOG_ERROR, "%s:%d: mais de um 'P' no mapa", path, lineNo);
                    return false;
                }
                hasPlayer = true;
                lvl->spawnPlayer = TileToPixel(c, row);
            }
            else if (ch >= '0' && ch <= '9')
            {
                int id = ch - '0';

                if (pCount[id] >= 2)
                {
                    TraceLog(LOG_ERROR, "%s:%d: o digito '%c' aparece mais de 2 vezes", path, lineNo, ch);
                    return false;
                }
                pCol[id][pCount[id]] = c;
                pRow[id][pCount[id]] = row;
                pCount[id]++;
            }
            else if (ch >= 'a' && ch <= 'j')
            {
                if (lvl->blockCount >= MAX_MEM_BLOCKS)
                {
                    TraceLog(LOG_ERROR, "%s:%d: blocos demais (maximo %d)", path, lineNo, MAX_MEM_BLOCKS);
                    return false;
                }

                MemoryBlock *b = &lvl->memoryBlock[lvl->blockCount++];
                b->gridX = c;
                b->gridY = row;
                b->pixelPosition = TileToPixel(c, row);
                b->spawnPosition = b->pixelPosition;
                b->radius = 14.0f;
                b->blockValue = ch - 'a' + 1;
                b->blockColor = ValueColor(b->blockValue);
            }
            else if (ch >= 'A' && ch <= 'J')
            {
                if (lvl->blockSlots >= MAX_SLOT_BLOCKS)
                {
                    TraceLog(LOG_ERROR, "%s:%d: slots demais (maximo %d)", path, lineNo, MAX_SLOT_BLOCKS);
                    return false;
                }

                MemorySlot *s = &lvl->memorySlot[lvl->blockSlots++];
                s->gridX = c;
                s->gridY = row;
                s->pixelPosition = (Vector2){c * TILE_SIZE, row * TILE_SIZE};
                s->spawnPosition = s->pixelPosition;
                s->hitbox = (Rectangle){s->pixelPosition.x, s->pixelPosition.y, TILE_SIZE, TILE_SIZE};
                s->slotValue = ch - 'A' + 1;
                s->slotColor = ValueColor(s->slotValue);
            }
            else
            {
                TraceLog(LOG_ERROR, "%s:%d: coluna %d: caractere invalido '%c'", path, lineNo, c + 1, ch);
                return false;
            }
        }
        row++;
    }

    // ---- VALIDAÇÕES FINAIS ----
    if (row != MAP_ROWS)
    {
        TraceLog(LOG_ERROR, "%s: %d linhas de mapa (esperado %d)", path, row, MAP_ROWS);
        return false;
    }

    if (!hasPlayer)
    {
        TraceLog(LOG_ERROR, "%s: faltou o 'P' (inicio do jogador)", path);
        return false;
    }

    if (lvl->blockSlots == 0)
    {
        TraceLog(LOG_ERROR, "%s: a fase precisa de pelo menos 1 slot (A-J)", path);
        return false;
    }

    // Todo slot precisa ter ao menos um bloco do mesmo valor, senão a fase é impossível
    for (int i = 0; i < lvl->blockSlots; i++)
    {
        bool found = false;

        for (int j = 0; j < lvl->blockCount; j++)
        {
            if (lvl->memoryBlock[j].blockValue == lvl->memorySlot[i].slotValue)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            TraceLog(LOG_ERROR, "%s: o slot '%c' nao tem o bloco '%c' correspondente", path,
                     'A' + lvl->memorySlot[i].slotValue - 1, 'a' + lvl->memorySlot[i].slotValue - 1);
            return false;
        }
    }

    // Monta os inimigos (as linhas @enemy podem vir antes ou depois do mapa)
    for (int id = 0; id < MAX_ENEMIES; id++)
    {
        if (pCount[id] == 0)
            continue;

        if (pCount[id] != 2)
        {
            TraceLog(LOG_ERROR, "%s: o inimigo %d precisa de exatamente 2 pontos no mapa (achei %d)",
                     path, id, pCount[id]);
            return false;
        }

        int ac = pCol[id][0], ar = pRow[id][0];
        int bc = pCol[id][1], br = pRow[id][1];

        if (ac != bc && ar != br)
        {
            TraceLog(LOG_ERROR, "%s: a patrulha do inimigo %d precisa ser reta (mesma linha ou coluna)",
                     path, id);
            return false;
        }

        if (!PatrolIsClear(lvl, ac, ar, bc, br))
        {
            TraceLog(LOG_ERROR, "%s: a patrulha do inimigo %d atravessa uma parede", path, id);
            return false;
        }

        Enemy *e = &lvl->enemy[lvl->enemyCount++];
        e->type = eType[id];
        e->a = TileToPixel(ac, ar);
        e->b = TileToPixel(bc, br);
        e->pos = e->a;
        e->speed = eSpeed[id];
        e->half = 14.0f;
        e->toB = true;
    }

    return true;
}

bool LoadLevel(const char *path, Level *lvl)
{
    FILE *f = fopen(path, "r");
    if (!f)
    {
        TraceLog(LOG_ERROR, "LEVEL: nao foi possivel abrir %s", path);
        return false;
    }

    memset(lvl, 0, sizeof *lvl);
    lvl->timeLimit = 60.0f; // valor padrão
    snprintf(lvl->levelName, sizeof lvl->levelName, "Fase sem nome");

    bool ok = ParseLevel(f, path, lvl);
    fclose(f);
    return ok;
}

// ---------------------------------------------------------------------------------------------
// INIMIGOS

void UpdateEnemies(Level *lvl, float dt)
{
    for (int i = 0; i < lvl->enemyCount; i++)
    {
        Enemy *e = &lvl->enemy[i];

        Vector2 target = e->toB ? e->b : e->a;
        Vector2 d = Vector2Subtract(target, e->pos);
        float dist = Vector2Length(d);
        float step = e->speed * dt;

        if (dist <= step)
        {
            // Chegou no extremo: encaixa e volta
            e->pos = target;
            e->toB = !e->toB;
        }
        else
        {
            e->pos = Vector2Add(e->pos, Vector2Scale(d, step / dist));
        }
    }
}

int CountFilledSlots(const Level *lvl)
{
    int n = 0;

    for (int i = 0; i < lvl->blockSlots; i++)
    {
        if (lvl->memorySlot[i].isFilled)
            n++;
    }

    return n;
}

// ---------------------------------------------------------------------------------------------
// DESENHO

void DrawLevel(const Level *lvl)
{
    for (int r = 0; r < MAP_ROWS; r++)
    {
        for (int c = 0; c < MAP_COLS; c++)
        {
            int x = c * TILE_SIZE;
            int y = r * TILE_SIZE;

            switch (lvl->tileMap[r][c])
            {
            case TILE_WALL:
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, (Color){15, 18, 28, 255});
                break;
            case TILE_BUS:
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, (Color){200, 160, 0, 255});
                break;
            default:
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, (Color){46, 70, 89, 255});
                break;
            }

            DrawRectangleLines(x, y, TILE_SIZE, TILE_SIZE, Fade(BLACK, 0.25f));
        }
    }

    // Trilha de cada patrulha (ajuda o jogador a ler o caminho do inimigo)
    for (int i = 0; i < lvl->enemyCount; i++)
    {
        const Enemy *e = &lvl->enemy[i];
        Color c = (e->type == ENEMY_KILL) ? RED : (e->type == ENEMY_STUN) ? YELLOW
                                                                          : MAGENTA;
        DrawLineV(e->a, e->b, Fade(c, 0.35f));
    }
}

void DrawEnemies(const Level *lvl)
{
    for (int i = 0; i < lvl->enemyCount; i++)
    {
        const Enemy *e = &lvl->enemy[i];

        Color c = RED;
        const char *label = "K";

        if (e->type == ENEMY_STUN)
        {
            c = YELLOW;
            label = "S";
        }
        else if (e->type == ENEMY_REVERSE)
        {
            c = MAGENTA;
            label = "R";
        }

        Rectangle box = {e->pos.x - e->half, e->pos.y - e->half, e->half * 2.0f, e->half * 2.0f};

        DrawRectangleRec(box, c);
        DrawRectangleLinesEx(box, 2.0f, BLACK);

        int w = MeasureText(label, 18);
        DrawText(label, (int)e->pos.x - w / 2, (int)e->pos.y - 9, 18, BLACK);
    }
}