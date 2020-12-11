#define m_chanelNum 22
#define m_modelOutH 241
#define m_modelOutW 321
#define m_fitModelOutputSizeH 233
#define m_fitModelOutputSizeW 321

__kernel void extractMaxProbClass(__global float* data,__global uchar* Label_data)
{
    int idx = get_global_id(0);
    int idy = get_global_id(1);
    int index = idy*m_modelOutW + idx;
    int class_table[22] = {0,1,0,2,3,4,5,6,7,0,8,9,10,11,12,255,0,13,14,0,15,16}; 
    if((idx<m_fitModelOutputSizeW) && (idy<m_fitModelOutputSizeH))
    {
        float max_prob = 0.0;
        int max_index = -1;
        for(int channel = 0; channel < m_chanelNum ; channel++)
        {
            if (data[(idy*m_modelOutW+idx)*m_chanelNum+channel] > max_prob)
            {
                max_prob = data[(idy*m_modelOutW+idx)*m_chanelNum+channel];
                max_index = class_table[channel];
            }
        }
        Label_data[idy*m_fitModelOutputSizeW+idx] = max_index;
    }
}