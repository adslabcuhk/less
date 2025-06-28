#include "Util.hh"

void Util::printGFMatrix(int *matrix, int row, int col, int w)
{
    for (int i = 0; i < row; i++)
    {
        printf("    ");
        for (int j = 0; j < col; j++)
        {
            int a = matrix[i * col + j];
            if (a == 0)
            {
                printf("-- ");
            }
            else
                printf("%02x ", a);
        }
        printf("\n");
    }
    printf("\n\n");
}