#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <papi.h>
#include <stdio.h>
#include <time.h>

using namespace std;

#define SYSTEMTIME clock_t

enum MultType {
    ORIGINAL,
    LINE,
    BLOCK_ORIGINAL,
    BLOCK_LINE
};

void calcOriginalMult(double *pha, double *phb, double *phc, int lines, int cols) {
    int i, j, k;
    int temp;

    for (i = 0; i < lines; i++)
    {
        for (j = 0; j < cols; j++)
        {
            temp = 0;
            for (k = 0; k < lines; k++)
            {
                temp += pha[i * lines + k] * phb[k * cols + j];
            }
            phc[i * lines + j] = temp;
        }
    }
}

void calcLineMult(double *pha, double *phb, double *phc, int lines, int cols) {
    int i, j, k;
    
    for (i = 0; i < lines; i++)
    {
        for (k = 0; k < cols; k++)
        {
            for (j = 0; j < lines; j++)
            {
                phc[i * lines + j] += pha[i * lines + k] * phb[k * cols + j];
            }
        }
    }
}

void calcOriginalBlockMult(double *pha, double *phb, double *phc, int lines, int cols, int blockSize) {
    int nBlocks = lines / blockSize;
    int i, j, k, i1, j1, k1;
    int iBase, jBase, kBase;

    for (i = 0; i < nBlocks; i++) { // iterate blocks vertically
        iBase = i * blockSize;
        for (j = 0; j < nBlocks; j++) { // iterate blocks horizontally
            jBase = j * blockSize;
            // compute each block
            for (k = 0; k < nBlocks; k++) { // for each resulting block, iterate sum's parcels (each parcel is a multiplication of block * block)
                kBase = k * blockSize;
                // compute a block's component (block * block)
                for (i1 = 0; i1 < blockSize; i1++) {
                    for (j1 = 0; j1 < blockSize; j1++) {
                        for (k1 = 0; k1 < blockSize; k1++) {
                            phc[(iBase + i1) * lines + (jBase + j1)] += pha[(iBase + i1) * lines + (kBase + k1)] * phb[(kBase + k1) * cols + (jBase + j1)];
                        }
                    }
                }
            }
        }
    }
}

void calcLineBlockMult(double *pha, double *phb, double *phc, int lines, int cols, int blockSize) {
    int nBlocks = lines / blockSize;
    int i, j, k, i1, j1, k1;
    int iBase, jBase, kBase;

    for (i = 0; i < nBlocks; i++) { // iterate blocks vertically
        iBase = i * blockSize;
        for (j = 0; j < nBlocks; j++) { // iterate blocks horizontally
            jBase = j * blockSize;
            // compute each block
            for (k = 0; k < nBlocks; k++) { // for each resulting block, iterate sum's parcels (each parcel is a multiplication of block * block)
                kBase = k * blockSize;
                // compute a block's component (block * block)
                for (i1 = 0; i1 < blockSize; i1++) {
                    for (k1 = 0; k1 < blockSize; k1++) {
                        for (j1 = 0; j1 < blockSize; j1++) {
                            phc[(iBase + i1) * lines + (jBase + j1)] += pha[(iBase + i1) * lines + (kBase + k1)] * phb[(kBase + k1) * cols + (jBase + j1)];
                        }
                    }
                }
            }
        }
    }
}

void OnMult(int lines, int cols, enum MultType multType, int blockSize) {
    SYSTEMTIME Time1, Time2;

    double *pha, *phb, *phc;

    pha = (double *)malloc((lines * lines) * sizeof(double));
    phb = (double *)malloc((lines * lines) * sizeof(double));
    phc = (double *)malloc((lines * lines) * sizeof(double));

    int i, j, k;

    for (i = 0; i < lines; i++) {
        for (j = 0; j < lines; j++) {
            pha[i * lines + j] = (double) 1.0;
        }
    }

    for (i = 0; i < cols; i++) {
        for (j = 0; j < cols; j++) {
            phb[i * cols + j] = (double) (i + 1);
        }
    }

    Time1 = clock();

    switch (multType) {
        case ORIGINAL:
            calcOriginalMult(pha, phb, phc, lines, cols);
            break;
        case LINE:
            calcLineMult(pha, phb, phc, lines, cols);
            break;
        case BLOCK_ORIGINAL:
            calcOriginalBlockMult(pha, phb, phc, lines, cols, blockSize);
            break;
        case BLOCK_LINE:
            calcLineBlockMult(pha, phb, phc, lines, cols, blockSize);
            break;
        default:
            cerr << "Invalid mult type" << endl;
    }

    Time2 = clock();

    char st[100];
    sprintf(st, "Time: %3.3f seconds\n",
            (double)(Time2 - Time1) / CLOCKS_PER_SEC);
    cout << st;

    // cout << "Result matrix: " << endl;
    for (i = 0; i < 1; i++)
    {
        for (j = 0; j < min(10, cols); j++)
            cout << phc[j] << " ";
    }
    cout << endl;

    free(pha);
    free(phb);
    free(phc);
}

void handle_error(int retval)
{
    printf("PAPI error %d: %s\n", retval, PAPI_strerror(retval));
    exit(1);
}

void init_papi()
{
    int retval = PAPI_library_init(PAPI_VER_CURRENT);
    if (retval != PAPI_VER_CURRENT && retval < 0)
    {
        printf("PAPI library version mismatch!\n");
        exit(1);
    }
    if (retval < 0)
        handle_error(retval);

    std::cout << "PAPI Version Number: MAJOR: " << PAPI_VERSION_MAJOR(retval)
              << " MINOR: " << PAPI_VERSION_MINOR(retval)
              << " REVISION: " << PAPI_VERSION_REVISION(retval) << "\n";
}

int main(int argc, char *argv[])
{

    char c;
    int lin, col, nt = 1;
    int op;

    int EventSet = PAPI_NULL;
    long long values[2];
    int ret;

    ret = PAPI_library_init(PAPI_VER_CURRENT);
    if (ret != PAPI_VER_CURRENT)
        std::cout << "FAIL" << endl;

    ret = PAPI_create_eventset(&EventSet);
    if (ret != PAPI_OK)
        cout << "ERRO: create eventset" << endl;

    ret = PAPI_add_event(EventSet, PAPI_L1_DCM);
    if (ret != PAPI_OK)
        cout << "ERRO: PAPI_L1_DCM" << endl;

    ret = PAPI_add_event(EventSet, PAPI_L2_DCM);
    if (ret != PAPI_OK)
        cout << "ERRO: PAPI_L2_DCM" << endl;

    op = 1;
    do
    {
        cout << endl
             << "1. Original Multiplication" << endl;
        cout << "2. Line Multiplication" << endl;
        cout << "3. Block Multiplication - Original" << endl;
        cout << "4. Block Multiplication - Line" << endl;
        cout << "5. Experience Multiplication" << endl;
        cout << "Selection?: ";
        cin >> op;
        if (op == 0)
            break;
        printf("Dimensions: lins cols ? ");
        cin >> lin >> col;

        // Start counting
        ret = PAPI_start(EventSet);
        if (ret != PAPI_OK)
            cout << "ERRO: Start PAPI" << endl;

        int blockSize;

        switch (op) {
        case 1:
            OnMult(lin, col, ORIGINAL, -1);
            break;
        case 2:
            OnMult(lin, col, LINE, -1);
            break;
        case 3:
            cout << "Block size: ";
            cin >> blockSize;
            OnMult(lin, col, BLOCK_ORIGINAL, blockSize);
            break;
        case 4:
            cout << "Block size: ";
            cin >> blockSize;
            OnMult(lin, col,BLOCK_LINE, blockSize);
            break;
        case 5:
            for(int h=600;h<=3000;h+=400){
                cout << "Size: "<< h<< endl;
                // OnMult(h, h);
                // OnMultLine(h,h);
            }
            break;
        default:
            cout << "Oops" << endl;
        }
        ret = PAPI_stop(EventSet, values);
        if (ret != PAPI_OK)
            cout << "ERRO: Stop PAPI" << endl;
        printf("L1 DCM: %lld \n", values[0]);
        printf("L2 DCM: %lld \n", values[1]);

        ret = PAPI_reset(EventSet);
        if (ret != PAPI_OK)
            std::cout << "FAIL reset" << endl;

    } while (op != 0);

    ret = PAPI_remove_event(EventSet, PAPI_L1_DCM);
    if (ret != PAPI_OK)
        std::cout << "FAIL remove event" << endl;

    ret = PAPI_remove_event(EventSet, PAPI_L2_DCM);
    if (ret != PAPI_OK)
        std::cout << "FAIL remove event" << endl;

    ret = PAPI_destroy_eventset(&EventSet);
    if (ret != PAPI_OK)
        std::cout << "FAIL destroy" << endl;
}
