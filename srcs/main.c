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
    void    *exit_close;
    void    *exit_open;
    void    *treasure;
    void    *floor;
    void    *player_img;

    int     player_x;
    int     player_y;

    int offset_x;   
    int offset_y;

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


void center_map(t_data *data)
{
    int map_width = data->cols * TILE_SIZE;
    int map_height = data->rows * TILE_SIZE;

    data->offset_x = (WINDOW_WIDTH - map_width - 500) / 2;
    data->offset_y = (WINDOW_HEIGHT - map_height - 400) / 2;

    if (data->offset_x < 0) data->offset_x = 0;
    if (data->offset_y < 0) data->offset_y = 0;
}

void render_background(t_data *data)
{
    for (int y = 0; y < WINDOW_HEIGHT; y += TILE_SIZE)
    {
        for (int x = 0; x < WINDOW_WIDTH; x += TILE_SIZE)
        {
            mlx_put_image_to_window(data->mlx_ptr, data->win_ptr,
                data->background, x, y);
        }
    }
}

// Функція для ручної прозорості персонажа
void put_image_with_transparency(t_data *data, void *img, int img_w, int img_h, int x_pos, int y_pos)
{
    int bpp, size_line, endian;
    char *addr = mlx_get_data_addr(img, &bpp, &size_line, &endian);

    int transparent_color = 0xFF00FF; // колір фону, який треба пропускати

    for (int j = 0; j < img_h; j++)
    {
        for (int i = 0; i < img_w; i++)
        {
            int *pixel = (int *)(addr + j * size_line + i * (bpp / 8));
            if (*pixel != transparent_color)
            {
                mlx_pixel_put(data->mlx_ptr, data->win_ptr,
                    x_pos + i,
                    y_pos + j,
                    *pixel);
            }
        }
    }
}

void render_map(t_data *data)
{
    render_background(data);

    for (int y = 0; y < data->rows; y++)
{
    for (size_t x = 0; x < strlen(data->map[y]); x++)
    {
        char c = data->map[y][x];

        if (c == '1')
            mlx_put_image_to_window(data->mlx_ptr, data->win_ptr, data->wall,
                                    data->offset_x + x * TILE_SIZE, data->offset_y + y * TILE_SIZE);
        else if (c == 'E')
        {
            mlx_put_image_to_window(data->mlx_ptr, data->win_ptr, data->floor,
                                    data->offset_x + x * TILE_SIZE, data->offset_y + y * TILE_SIZE);
            put_image_with_transparency(data, data->exit_close, data->player_w, data->player_h,
                                        data->offset_x + x * TILE_SIZE, data->offset_y + y * TILE_SIZE);
        }
        else if (c == 'C')
        {
            mlx_put_image_to_window(data->mlx_ptr, data->win_ptr, data->floor,
                                    data->offset_x + x * TILE_SIZE, data->offset_y + y * TILE_SIZE);
            put_image_with_transparency(data, data->treasure, data->player_w, data->player_h,
                                        data->offset_x + x * TILE_SIZE, data->offset_y + y * TILE_SIZE);
        }
        else
            mlx_put_image_to_window(data->mlx_ptr, data->win_ptr, data->floor,
                                    data->offset_x + x * TILE_SIZE, data->offset_y + y * TILE_SIZE);
    }
}

    put_image_with_transparency(data, data->player_img, data->player_w, data->player_h,
                            data->offset_x + data->player_x, data->offset_y + data->player_y);

}

int handle_keypress(int keysym, t_data *data)
{
    if (keysym == XK_Escape)
    {
        mlx_destroy_window(data->mlx_ptr, data->win_ptr);
        data->win_ptr = NULL;
        exit(0);
        return 0;
    }

    int new_x = data->player_x;
    int new_y = data->player_y;

    if (keysym == XK_a || keysym == XK_A)  
        new_x -= TILE_SIZE;
    if (keysym == XK_d || keysym == XK_D) 
        new_x += TILE_SIZE;
    if (keysym == XK_w|| keysym == XK_W)    
        new_y -= TILE_SIZE;
    if (keysym == XK_s || keysym == XK_S)  
        new_y += TILE_SIZE;

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

    data.win_ptr = mlx_new_window(data.mlx_ptr, WINDOW_WIDTH, WINDOW_HEIGHT, "so_long");
    if (!data.win_ptr) return MLX_ERROR;

    data.background = mlx_xpm_file_to_image(data.mlx_ptr, "./sprites/background.xpm", &w, &h);
    data.wall = mlx_xpm_file_to_image(data.mlx_ptr, "./sprites/wall.xpm", &w, &h);
    data.floor = mlx_xpm_file_to_image(data.mlx_ptr, "./sprites/floor.xpm", &w, &h);
    data.exit_close = mlx_xpm_file_to_image(data.mlx_ptr, "./sprites/exit_close.xpm", &w, &h);
    data.treasure = mlx_xpm_file_to_image(data.mlx_ptr, "./sprites/treasure.xpm", &w, &h);
    data.exit_open = mlx_xpm_file_to_image(data.mlx_ptr, "./sprites/steps.xpm", &w, &h);
    data.player_img = mlx_xpm_file_to_image(data.mlx_ptr, "./sprites/player.xpm", &data.player_w, &data.player_h);

    if (!data.background || !data.wall || !data.player_img)
    {
        printf("Erreur chargement images\n");
        return 1;
    }

    data.map = read_map("maps/map.ber", &data.rows, &data.cols);
    if (!data.map)
    {
        printf("Impossible de lire map.ber\n");
        return 1;
    }

    center_map(&data);
    init_player(&data);
    render_map(&data);

    mlx_hook(data.win_ptr, KeyRelease, KeyReleaseMask, &handle_keypress, &data);
    mlx_loop(data.mlx_ptr);
    return 0;
}
