/*

Based on https://arxiv.org/pdf/2111.05592.pdf
not used in this plugin anymore

opcode Svar3,aaaaa,akk
setksmps 1
as1,as2 init 0,0
as,kK,kQ xin
kdiv = 1+kK/kQ+kK*kK
ahp = (as - (1/kQ+kK)*as1 - as2)/kdiv
au = ahp*kK
abp = au + as1
as1 = au + abp
au = abp*kK
alp = au + as2
as2 = au + alp
xout ahp,abp,alp,
ahp+alp,ahp+alp+(1/Q)*abp
endop



The allpass was a scam, does not work as expected

*/

class ExtendedSVF
{
private:
    double as1, as2, kK, kQ, kdiv, ahp, au, abp, alp;
    double x;

public:
    ExtendedSVF()
    {
        as1 = 0;
        as2 = 0;
        kK = 0;
        kQ = 0;
        kdiv = 0;
        ahp = 0;
        au = 0;
        abp = 0;
        alp = 0;
        x = 0;
    }

    void setKK(double newKK)
    {
        kK = newKK;
    }

    void setKQ(double newKQ)
    {
        kQ = newKQ;
    }

    void processSample(float as)
    {
        x = as;
        kdiv = 1 + kK / kQ + kK * kK;
        ahp = (as - (1 / kQ + kK) * as1 - as2) / kdiv;
        au = ahp * kK;
        abp = au + as1;
        as1 = au + abp;
        au = abp * kK;
        alp = au + as2;
        as2 = au + alp;
    }

    double getHighPass()
    {
        return ahp;
    }

    double getBandPass()
    {
        return abp;
    }

    double getLowPass()
    {
        return alp;
    }

    double getBandReject()
    {
        return ahp + alp;
    }

    double getAllPass()
    {
        return ahp + alp + (1 / kQ) * abp;
        // return x - (1/kQ)*abp;
    }
};
