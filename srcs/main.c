#include <stdlib.h>
#include <X11/X.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "../minilibx-linux/mlx.h"

#define WINDOW_WIDTH 1900
#define WINDOW_HEIGHT 1200
#define TILE_SIZE 64
#define MLX_ERROR 1

typedef struct s_data
{
    void    *mlx_ptr;
    void    *win_ptr;

    void    *background;
    void    *wall;
    void    *player_img;

    int     player_x;
    int     player_y;

    int     player_w;
    int     player_h;

    char    **map;
    int     rows;
    int     cols;

}   t_data;

char **read_map(const char *filename, int *rows, int *cols)
{
    FILE *file = fopen(filename, "r");
    if (!file) return NULL;

    char **map = NULL;
    char buffer[1024];
    int r = 0;
    int max_col = 0;

    while (fgets(buffer, sizeof(buffer), file))
    {
        int len = strlen(buffer);
        if (buffer[len-1] == '\n') buffer[len-1] = '\0';
        len = strlen(buffer);
        if (len > max_col) max_col = len;

        map = realloc(map, sizeof(char*) * (r + 1));
        map[r] = malloc(len + 1);
        strcpy(map[r], buffer);
        r++;
    }

    fclose(file);
    *rows = r;
    *cols = max_col;
    return map;
}

void init_player(t_data *data)
{
    for (int y = 0; y < data->rows; y++)
    {
        for (size_t x = 0; x < strlen(data->map[y]); x++)
        {
            if (data->map[y][x] == 'P')
            {
                data->player_x = x * TILE_SIZE;
                data->player_y = y * TILE_SIZE;
                return;
            }
        }
    }
}

void render_map(t_data *data)
{
    for (int y = 0; y < data->rows; y++)
    {
        for (size_t x = 0; x < strlen(data->map[y]); x++)
        {
            char c = data->map[y][x];
            void *img_to_put = NULL;
            if (c == '1')
                img_to_put = data->wall;
            else
                img_to_put = data->background;

            mlx_put_image_to_window(data->mlx_ptr, data->win_ptr, img_to_put, x*TILE_SIZE, y*TILE_SIZE);
        }
    }

    mlx_put_image_to_window(data->mlx_ptr, data->win_ptr,
                            data->player_img,
                            data->player_x, data->player_y);
}

int handle_keypress(int keysym, t_data *data)
{
    if (keysym == XK_Escape)
    {
        mlx_destroy_window(data->mlx_ptr, data->win_ptr);
        data->win_ptr = NULL;
		exit(0);
        return (0);
    }

    int new_x = data->player_x;
    int new_y = data->player_y;

    if (keysym == XK_Left)  new_x -= TILE_SIZE;
    if (keysym == XK_Right) new_x += TILE_SIZE;
    if (keysym == XK_Up)    new_y -= TILE_SIZE;
    if (keysym == XK_Down)  new_y += TILE_SIZE;

    int tile_x = new_x / TILE_SIZE;
    int tile_y = new_y / TILE_SIZE;
    if (tile_x >= 0 && tile_x < data->cols &&
        tile_y >= 0 && tile_y < data->rows &&
        data->map[tile_y][tile_x] != '1')
    {
        data->player_x = new_x;
        data->player_y = new_y;
    }

    render_map(data);
    return 0;
}

int main(void)
{
    t_data data;
    int w, h;

    data.mlx_ptr = mlx_init();
    if (!data.mlx_ptr) return MLX_ERROR;

    data.win_ptr = mlx_new_window(data.mlx_ptr, WINDOW_WIDTH, WINDOW_HEIGHT, "Game");
    if (!data.win_ptr) return MLX_ERROR;

    data.background = mlx_xpm_file_to_image(data.mlx_ptr, "./sprites/water.xpm", &w, &h);
    data.wall = mlx_xpm_file_to_image(data.mlx_ptr, "./sprites/border.xpm", &w, &h);
    data.player_img = mlx_xpm_file_to_image(data.mlx_ptr, "./sprites/C0.xpm", &data.player_w, &data.player_h);

    if (!data.background || !data.wall || !data.player_img)
    {
        printf("Erreur chargement images\n");
        return 1;
    }

    // lire map
    data.map = read_map("maps/map.ber", &data.rows, &data.cols);
    if (!data.map)
    {
        printf("Impossible de lire map.ber\n");
        return 1;
    }

    init_player(&data);
    render_map(&data);

    mlx_hook(data.win_ptr, KeyRelease, KeyReleaseMask, &handle_keypress, &data);
    mlx_loop(data.mlx_ptr);
    return (0);
}
