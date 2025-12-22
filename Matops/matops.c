#include <stdio.h>
#include <math.h>
int main()
{   
    int password = 12345;
    int user_input_password = 0;
    int sys_choice = 0;
    int try = 0;
    int flag = 0;
    while (try != 5)
    {
    if (flag == 0)
    {
        printf("\t\t\t\t\t\t------------------\n");
        printf("\t\t\t\t\t\tWELCOME TO MATOPS\n");
        printf("\t\t\t\t\t\t------------------\n");
        printf("ENTER PASSWORD TO LOG IN: ");
        scanf("%d", &user_input_password);
    }

    if (password == user_input_password)
    {
        try = 0;
        printf("------------------\n");
        printf("WELCOME TO MATOPS\n");
        printf("------------------\n");
        int choice;
        printf("CHOOSE OPERATIONS:\n");
        printf("1. MATRIX MULTIPLICATION\n");
        printf("2. DETERMINANT (upto 3 * 3 matrix)\n");
        printf("3. INVERSE\n");
        printf("4. CHARACTERSTIC EQUATION\n");
        printf("5. EIGEN VALUE\n");
        printf("6. EIGEN VECTOR\n");
        printf("7. TRANSPOSE OF A MATRIX\n");
        printf("8. SYSTEM SETTINGS\n");
        printf("0. EXIT\n");
        printf("----------------------\n");
        printf("ENTER YOUR CHOICE:");
        scanf("%d", &choice);
        printf("----------------------\n");
        int detA;
        if (choice == 1)
        {
            unsigned int r1, c1, r2, c2;
            printf("ENTER THE NUMBER OF ROWS OF MATRIX A:-\n");       
            scanf("%u", &r1);
            printf("\nENTER THE NUMBER OF COLUMN OF MATRIX A:-\n");
            scanf("%u", &c1);
            printf("\nENTER THE NUMBER OF ROWS IN MATRIX B:-\n");
            scanf("%u", &r2);
            printf("\nENTER THE NUMBER OF COLUMN IN MATRIX B:-\n");
            scanf("%u", &c2);
            if (r1 == 0 || c1 == 0 || r2 == 0 || c2 == 0)           
            {
                printf("---------------------------------------------\n");
                printf("\nROWS AND COLUMNS MUST BE GREATER THAN ZERO.\n");
                printf("---------------------------------------------\n");
            }
            else if (c1 != r2)
            {
                printf("--------------------------------------------------------\n");
                printf("SORRY! THE MULTIPLICATION OF THE MATRIX IS NOT POSSIBLE.\n");
                printf("--------------------------------------------------------\n");
            }
            else
            {
                int arr1[r1][c1],arr2[r2][c2],arr3[r1][c2], i, j, k;
                printf("\nENTER THE ELEMENTS OF MATRIX A:-\n");
                for (i = 0; i < r1; i++)
                {
                    for (j = 0; j < c1; j++)
                    {
                        scanf("%d", &arr1[i][j]);
                    }
                }
                printf("\nENTER THE ELEMENTS OF MATRIX B:-\n");
                for (i = 0; i < r2; i++)
                {
                    for (j = 0; j < c2; j++)
                    {
                        scanf("%d", &arr2[i][j]);
                    }
                }
                printf("------------------------------------------------------------\n");
                printf("\nTHE RESULT OF MULTIPLICATION OF MATRIX A AND MATRIX B IS:-\n");
                for (i = 0; i < r1; i++)
                {   

                    for (j = 0; j < c2; j++)                   
                    {
                        arr3[i][j] = 0;
                        for (k = 0; k < c1; k++)
                        {
                            arr3[i][j] += arr1[i][k] * arr2[k][j];
                        }
                        printf("%d\t", arr3[i][j]);
                }
                    printf("\n");
            
                }
                printf("------------------------------------------------------------\n");
            }
            flag = 1;
        }
        else if (choice == 2)
        {
            unsigned int r1, c1;
            printf("ENTER THE NUMBER OF ROWS OF MATRIX A:-\n");
            scanf("%u", &r1);     
            printf("\nENTER THE NUMBER OF COLUMN OF MATRIX A:-\n");
            scanf("%u", &c1);
            int arr1[r1][c1];
            if (r1 != c1)
            {
                printf("--------------------------\n");
                printf("DETERMINANT DOES NOT EXIST\n");
                printf("--------------------------\n");
            }
            else if (r1 == c1)
            {
                printf("ENTER THE ELEMENTS OF MAT A:\n");
                for (int i = 0; i < r1; i++)
                {
                    for (int j = 0; j < c1; j++)
                    {
                        scanf("%d", &arr1[i][j]);
                    }
                }
                if (r1 == 1)
                {
                    detA = arr1[0][0];
                    printf("---------------------------------\n");
                    printf("DETERMINANAT OF MATRIX A IS : %d\n", detA);
                    printf("----------------------------------\n");
                }
                else if (r1 == 2)
                {
                    detA = arr1[0][0] * arr1[1][1] - (arr1[0][1] * arr1[1][0]);
                    printf("-------------------------------\n");
                    printf("DETERMINANT OF MATRIX A IS : %d\n", detA); 
                    printf("-------------------------------\n");
                }
                else if (r1 == 3)
                {
                    detA = arr1[0][0]*(arr1[1][1]*arr1[2][2] - arr1[1][2]*arr1[2][1])
                          - arr1[0][1]*(arr1[1][0]*arr1[2][2] - arr1[1][2]*arr1[2][0])
                          + arr1[0][2]*(arr1[1][0]*arr1[2][1] - arr1[1][1]*arr1[2][0]);
                    printf("------------------------------\n");
                    printf("DETERMINANT OF MATRIX A IS :%d\n", detA);
                    printf("------------------------------\n");

                }
            }
            flag = 1;

        }
        else if (choice == 3)
        {
            unsigned int r1, c1;
            printf("ENTER THE NUMBER OF ROWS OF MATRIX A:-\n");      
            scanf("%u", &r1);
            printf("\nENTER THE NUMBER OF COLUMN OF MATRIX A:-\n");
            scanf("%u", &c1);
            float arr1[r1][c1];
            float inv[r1][c1];
            if (r1 != c1)
            {
                printf("----------------------\n");
                printf("INVERSE DOES NOT EXIST\n");
                printf("----------------------\n");
            }
            else if (r1 != 2 && r1 != 3 && r1 != 1)
            {
                printf("--------------------------------------\n");
                printf("ONLY 1X1, 2X2 AND 3X3 MATRIX SUPPORTED\n");
                printf("--------------------------------------\n");
            }
            else if (r1 == 1)
            {   
                printf("ENTER ELEMENTS OF MATRIX A:\n");
                for (int i = 0; i < r1; i++)
                {
                    for (int j = 0; j < c1; j++)
                    {
                        scanf("%f", &arr1[i][j]);
                    }
                }
                detA = arr1[0][0];
                inv[0][0] = 1/detA; 
                if (detA == 0)
                {
                    printf("----------------------\n");
                    printf("INVERSE DOES NOT EXIST\n");
                    printf("----------------------\n");
                }
                else if (detA != 0)
                {
                    printf("---------------------------\n");
                    printf("INVERSE OF MATRIX A IS : \n");
                    for (int i = 0; i < r1; i++)
                    {
                        for (int j = 0; j < c1; j++)
                        {
                            printf("%.2f\n", inv[i][j]);
                        }
                    }
                    printf("---------------------------\n");
                }          
            }
            else if (r1 == 2)
            {   printf("ENTER ELEMENTS OF MATRIX A:\n");
                for (int i = 0; i < r1; i++)
                {
                    for (int j = 0; j < c1; j++)
                    {
                        scanf("%f", &arr1[i][j]);
                    }
                }
                detA = arr1[0][0] * arr1[1][1] - (arr1[0][1] * arr1[1][0]);
                if (detA == 0)
                {
                    printf("----------------------\n");
                    printf("INVERSE DOES NOT EXIST\n");
                    printf("-----------------------\n");
                }
                else if (detA != 0)
                {
                    inv[0][0] = arr1[1][1]/detA;
                    inv[0][1] = -arr1[0][1]/detA;
                    inv[1][0] = -arr1[1][0]/detA;
                    inv[1][1] = arr1[0][0]/detA;
                    printf("---------------------------\n");
                    printf("INVERSE OF MATRIX A IS : \n");
                    for (int i = 0; i < r1; i++)
                    {
                        for (int j = 0; j < c1; j++)
                        {
                            printf("%.2f\t", inv[i][j]);
                        }
                        printf("\n");
                    }
                    printf("---------------------------\n");
                }
            }
            else if (r1 == 3)
            {
                printf("ENTER ELEMENTS OF MATRIX A:\n");
                for (int i = 0; i < r1; i++)
                {
                    for (int j = 0; j < c1; j++)
                    {
                        scanf("%f", &arr1[i][j]);
                    }
                }
                detA = arr1[0][0]*(arr1[1][1]*arr1[2][2] - arr1[1][2]*arr1[2][1])
                       - arr1[0][1]*(arr1[1][0]*arr1[2][2] - arr1[1][2]*arr1[2][0])
                       + arr1[0][2]*(arr1[1][0]*arr1[2][1] - arr1[1][1]*arr1[2][0]);
                if (detA == 0)
                {
                    printf("-----------------------\n");
                    printf("INVERSE DOES NOT EXIST\n");
                    printf("-----------------------\n");
                }
                else if (detA != 0)
                {
                    inv[0][0]=(arr1[1][1]*arr1[2][2]-arr1[1][2]*arr1[2][1])/detA;
                    inv[0][1]=-(arr1[0][1]*arr1[2][2]-arr1[0][2]*arr1[2][1])/detA;
                    inv[0][2]=(arr1[0][1]*arr1[1][2]-arr1[0][2]*arr1[1][1])/detA;

                    inv[1][0]=-(arr1[1][0]*arr1[2][2]-arr1[1][2]*arr1[2][0])/detA;
                    inv[1][1]=(arr1[0][0]*arr1[2][2]-arr1[0][2]*arr1[2][0])/detA;
                    inv[1][2]=-(arr1[0][0]*arr1[1][2]-arr1[0][2]*arr1[1][0])/detA;

                    inv[2][0]=(arr1[1][0]*arr1[2][1]-arr1[1][1]*arr1[2][0])/detA;
                    inv[2][1]=-(arr1[0][0]*arr1[2][1]-arr1[0][1]*arr1[2][0])/detA;
                    inv[2][2]=(arr1[0][0]*arr1[1][1]-arr1[0][1]*arr1[1][0])/detA;
                    printf("---------------------------\n");
                    for (int i = 0; i < r1; i++)
                    {
                    for (int j = 0; j < c1; j++)
                    {
                        printf("%.2f\t", inv[i][j]);
                    }
                    printf("\n");
                    }
                    printf("---------------------------\n");
                }
            }
            flag = 1;
        }
        else if (choice == 4)
        {
            unsigned int r1, c1;
            printf("ENTER THE NUMBER OF ROWS OF MATRIX A:-\n");      
            scanf("%u", &r1);
            printf("\nENTER THE NUMBER OF COLUMN OF MATRIX A:-\n");
            scanf("%u", &c1);
            int arr1[r1][c1];
            if (r1 != c1)
            {
                printf("------------------------------------------------------\n");
                printf("characteric equation is only defined for square matrix.\n");
                printf("------------------------------------------------------\n");
            }
            else if(r1 != 2 && r1 != 3)
            {
                printf("------------------------------------\n");
                printf("only 2x2 and 3x3 matrix is supported\n");
                printf("------------------------------------\n");
            }
            else if(r1 == 2)
            {
                printf("ENTER ELEMENTS OF MATRIX A:\n");
                for (int i = 0; i < r1; i++)
                {
                    for (int j = 0; j < c1; j++)
                    {
                        scanf("%d", &arr1[i][j]);
                    }
                }
                detA = arr1[0][0] * arr1[1][1] - (arr1[0][1] * arr1[1][0]);
                int traceA = arr1[0][0] + arr1[1][1];
                printf("--------------------------------------\n");
                printf("CHARATERSTIC EQUATION OF MATRIX A IS:\n");
                printf("λ² - %dλ + %d\n", traceA, detA);
                printf("---------------------------------------\n");
            }
            else if (r1 == 3)
            {   
                printf("ENTER ELEMENTS OF MATRIX A:\n");
                for (int i = 0; i < r1; i++)
                {
                    for (int j = 0; j < c1; j++)
                    {
                        scanf("%d", &arr1[i][j]);
                    }
                }
                detA = arr1[0][0]*(arr1[1][1]*arr1[2][2] - arr1[1][2]*arr1[2][1])
                       - arr1[0][1]*(arr1[1][0]*arr1[2][2] - arr1[1][2]*arr1[2][0])
                       + arr1[0][2]*(arr1[1][0]*arr1[2][1] - arr1[1][1]*arr1[2][0]);
                int traceA = arr1[0][0] + arr1[1][1] + arr1[2][2];
                int sum_principle_minors = arr1[1][1] * arr1[2][2] - arr1[1][2] * arr1[2][1]
                                           + arr1[0][0] * arr1[2][2] - arr1[0][2] * arr1[2][0]
                                           + arr1[0][0] * arr1[1][1] - arr1[0][1] * arr1[1][0];
                printf("---------------------------------------\n");
                printf("CHARATERSTIC EQUATION OF MATRIX A IS:\n");
                printf("λ³ - %dλ² + %dλ - %d\n", traceA, sum_principle_minors, detA);
                printf("----------------------------------------\n");
            }
            flag = 1;
        }
        else if (choice == 5)
        {
            unsigned int r1, c1;
            printf("ENTER THE NUMBER OF ROWS OF MATRIX A:-\n");      
            scanf("%u", &r1);
            printf("\nENTER THE NUMBER OF COLUMN OF MATRIX A:-\n");
            scanf("%u", &c1);
            int arr1[r1][c1];
            if (r1 != c1)
            {
                printf("-------------------------------------------------\n");
                printf("EIGEN VALUES ARE ONLY DEFINED FOR A SQUARE MATRIX\n");
                printf("-------------------------------------------------\n");
            }
            else if(r1 != 2)
            {
                printf("-----------------------------\n");
                printf("ONLY 2x2 MARRIX ARE SUPPORTED\n");
                printf("-----------------------------\n");
            }
            else if (r1 == 2)
            {
                printf("ENTER ELEMENTS OF MATRIX A:\n");
                for (int i = 0; i < r1; i++)
                {
                    for (int j = 0; j < c1; j++)
                    {
                        scanf("%d", &arr1[i][j]);
                    }
                }
                detA = arr1[0][0] * arr1[1][1] - (arr1[0][1] * arr1[1][0]);
                int traceA = arr1[0][0] + arr1[1][1];
                int discriminant = traceA * traceA - 4 * detA;
                float eigen_value1 = (traceA + sqrt(discriminant))/2;
                float eigen_value2 = (traceA - sqrt(discriminant))/2;
                printf("-------------------------------------\n");
                printf("EIGEN VALUE OF MATRIX A IS : %f and %f\n", eigen_value1, eigen_value2);
                printf("-------------------------------------\n");
            }
            flag = 1;
        }
        else if (choice == 6)
        {
            unsigned int r1, c1;
            printf("ENTER THE NUMBER OF ROWS OF MATRIX A:-\n");      
            scanf("%u", &r1);
            printf("\nENTER THE NUMBER OF COLUMN OF MATRIX A:-\n");
            scanf("%u", &c1);
            int arr1[r1][c1];
            if (r1 != c1)
            {
                printf("-------------------------------------------------\n");
                printf("EIGEN VECTOR ARE ONLY DEFINED FOR A SQUARE MATRIX\n");
                printf("-------------------------------------------------\n");
            }
            else if(r1 != 2)
            {
                printf("-----------------------------\n");
                printf("ONLY 2x2 MARRIX ARE SUPPORTED\n");
                printf("-----------------------------\n");
            }
            else if (r1 == 2)
            {
                printf("ENTER ELEMENTS OF MATRIX A:\n");
                for (int i = 0; i < r1; i++)
                {
                    for (int j = 0; j < c1; j++)
                    {
                        scanf("%d", &arr1[i][j]);
                    }
                }
                detA = arr1[0][0] * arr1[1][1] - (arr1[0][1] * arr1[1][0]);
                int traceA = arr1[0][0] + arr1[1][1];
                int discriminant = traceA * traceA - 4 * detA;
                float eigen_value1 = (traceA + sqrt(discriminant))/2;
                float eigen_value2 = (traceA - sqrt(discriminant))/2;
                printf("-------------------------------------------------\n");
                printf("EIGEN VECTOR FOR 2x2 MATRIX ARE:\n");
                printf("X₁ = %.2d\n", arr1[0][1]);
                printf("     %.2f\n", eigen_value1 - arr1[0][0]);
                printf("X₂ = %.2d\n", arr1[0][1]);
                printf("     %.2f\n", eigen_value2 - arr1[0][0]);
                printf("-------------------------------------------------\n");
            }
            flag = 1;
        }
        else if (choice == 7)
        {
            unsigned int r1, c1;
            printf("ENTER THE NUMBER OF ROWS OF MATRIX A:-\n");      
            scanf("%u", &r1);
            printf("\nENTER THE NUMBER OF COLUMN OF MATRIX A:-\n");
            scanf("%u", &c1);
            int arr1[r1][c1];
            printf("ENTER ELEMENTS OF MATRIX A:\n");
            for (int i = 0; i < r1; i++)
            {
                for (int j = 0; j < c1; j++)
                {
                    scanf("%d", &arr1[i][j]);
                }
            }
            printf("------------------------------------\n");
            printf("THE TRANSPOSE OF THE MATRIX A IS:\n");
            for (int i = 0; i < c1; i++)
            {
                for (int j = 0; j < r1; j++)
                {
                    printf("%d\t", arr1[j][i]);
                }
                printf("\n");
            }
            printf("------------------------------------\n");
            flag = 1;
        }
        else if (choice == 8)
        {
            while (sys_choice != 5)
            {
                printf("\t\t\t---------------\n");
                printf("\t\t\tSYSTEM SETTINGS\n");
                printf("\t\t\t---------------\n");
                printf("\t\t\t1. VERSION NUMBER\n");
                printf("\t\t\t2. AUTHOR\n");
                printf("\t\t\t3. MAINTAINER\n");
                printf("\t\t\t4. RECENT UPDATE DATE\n");
                printf("\t\t\t5. RETURN TO PREVIOUS MENU\n");
                printf("\t\t\t6. LOG OUT\n");
                printf("\t\t\tENTER THE CHOICE FOR SETTINGS: ");
                scanf("%d", &sys_choice);
                if (sys_choice == 1)
                {
                    printf("\t\t\t\t\t\t---------------------\n");
                    printf("\t\t\t\t\t\tVersion Number: 2.3.2\n");
                    printf("\t\t\t\t\t\t---------------------\n");
                }
                else if (sys_choice == 2)
                {
                    printf("\t\t\t\t\t\t--------------------\n");
                    printf("\t\t\t\t\t\tAuthor: ANANT RAJPUT\n");
                    printf("\t\t\t\t\t\t--------------------\n");
                }
                else if (sys_choice == 3)
                {
                    printf("\t\t\t\t\t\t---------------------------\n");
                    printf("\t\t\t\t\t\tMaintained by: ANANT RAJPUT\n");
                    printf("\t\t\t\t\t\t---------------------------\n");
                }
                else if (sys_choice == 4)
                {
                    printf("\t\t\t\t\t\t----------------------------------------\n");
                    printf("\t\t\t\t\t\tRecent Update Date: 23 . DECEMBER . 2025\n");
                    printf("\t\t\t\t\t\t----------------------------------------\n");
                }
                else if (sys_choice == 5)
                {
                    printf("\t\t\t\t\t\t--------------------------\n");
                    printf("\t\t\t\t\t\tReturned To Previous Menu.\n");
                    printf("\t\t\t\t\t\t--------------------------\n");
                    sys_choice = 0;
                    flag = 1;
                    break;
                }
                else if (sys_choice == 6)
                {
                    printf("\t\t\t\t\t\t-------------------------------------\n");
                    printf("\t\t\t\t\t\tLogged Out of the System Successfully.\n");
                    printf("\t\t\t\t\t\t--------------------------------------\n");
                    sys_choice = 0;
                    flag = 0;
                    break;
                }
            }

        }
        else if (choice == 0)
        {
            printf("\t\t\t\t\t\t-------------------------------\n");
            printf("\t\t\t\t\t\tEXITED THE SYSTEM SUCCESSFULLY.\n");
            printf("\t\t\t\t\t\t-------------------------------\n");
            break;
        }
        else
        {
            printf("\t\t\t\t\t\t---------------------------------------\n");
            printf("\t\t\t\t\t\tINVALID RESPOSE. VALID RESPONSE = 0 - 8\n");
            printf("\t\t\t\t\t\t---------------------------------------\n");
            flag = 1;
        }        
    }
    else
    {
        if (try < 4)
        {
            printf("\t\t\t\t\t\t----------------------------\n");
            printf("\t\t\t\t\t\tINCORRECT PASSWORD.TRY AGAIN\n");
            printf("\t\t\t\t\t\t----------------------------\n");
            try++;
        }
        else if (try == 4)
        {
            printf("\t\t\t\t\t\t------------------------------------\n");
            printf("\t\t\t\t\t\tINCORRECT PASSWORD.NO MORE TRIES LEFT\n");
            printf("\t\t\t\t\t\t------------------------------------\n");
            try++;
        }
    }
    
    }

    return 0;
}