#define Set_Bit(req,n)  (req = req | (1<<n))
#define Clear_Bit(req,n) (req = req & !(1<<n))
#define Get_Bit(req,n) (req & (1<<n))
#define Toggle_Bit(req,n) (req = req ^ (1<<n))