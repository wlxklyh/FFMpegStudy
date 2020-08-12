#pragma pack(1)

typedef unsigned int UINT32;
typedef short UINT16;

struct RIFF_HEADER
{
	char RiffID[4];  // 'R','I','F','F'
	UINT32 RiffSize;
	char RiffFormat[4]; // 'W','A','V','E'
};

struct WAVE_FORMAT
{
	UINT16 FormatTag;
	UINT16 Channels;
	UINT32 SamplesRate;
	UINT32 AvgBytesRate;
	UINT16 BlockAlign;
	UINT16 BitsPerSample;
};

void calformat(struct WAVE_FORMAT fmt)
{
	fmt.AvgBytesRate = fmt.SamplesRate * fmt.Channels * fmt.BitsPerSample/8;
	fmt.BlockAlign = fmt.Channels * fmt.BitsPerSample;
}

struct FMT_BLOCK
{
	char  FmtID[4]; // 'f','m','t',' '
	UINT32  FmtSize;
	WAVE_FORMAT wavFormat;
};
/*
struct FACT_BLOCK
{
	char  FactID[4]; // 'f','a','c','t'
	UINT32  FactSize;
};
*/
struct DATA_BLOCK
{
	char DataID[4]; // 'd','a','t','a'
	UINT32 DataSize;
};

struct WAVE_HEADER
{
	struct RIFF_HEADER header;
	struct FMT_BLOCK format;
	//struct FACT_BLOCK fact;
	struct DATA_BLOCK data;
};