#ifdef __cplusplus
  #define MY_EXTERN_C extern "C"
#else
  #define MY_EXTERN_C extern
#endif

#define MY_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
  MY_EXTERN_C const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

//formats 9.04
MY_DEFINE_GUID(CLSID_CZipHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x01, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CBZip2Handler,    0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x02, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CRarHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x03, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CArjHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x04, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CZHandler,        0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x05, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CLzhHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x06, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CFormat7z,        0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CCabHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x08, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CNsisHandler,     0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x09, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CLzmaHandler,     0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x0A, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CLzma86Handler,   0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x0B, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CXzHandler,       0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x0C, 0x00, 0x00);

MY_DEFINE_GUID(CLSID_CUdfHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xE0, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CXarHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xE1, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CMubHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xE2, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CHfsHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xE3, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CDmgHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xE4, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CCompoundHandler, 0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xE5, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CWimHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xE6, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CIsoHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xE7, 0x00, 0x00);

MY_DEFINE_GUID(CLSID_CChmHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xE9, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CSplitHandler,    0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xEA, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CRpmHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xEB, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CDebHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xEC, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CCpioHandler,     0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xED, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CTarHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xEE, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CGZipHandler,     0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xEF, 0x00, 0x00);

MY_DEFINE_GUID(CLSID_CNtfsHandler,     0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xD9, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CFatHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xDA, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CMbrHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xDB, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CVhdHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xDC, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CPeHandler,       0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xDD, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CElfHandler,      0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xDE, 0x00, 0x00);
MY_DEFINE_GUID(CLSID_CMachoHandler,    0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0xDF, 0x00, 0x00);


MY_DEFINE_GUID(IID_IInArchive, 0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x06, 0x00, 0x60, 0x00, 0x00);
MY_DEFINE_GUID(IID_IOutArchive, 0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x06, 0x00, 0xA0, 0x00, 0x00);
MY_DEFINE_GUID(IID_ISetProperties, 0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x06, 0x00, 0x03, 0x00, 0x00);
MY_DEFINE_GUID(IID_ISequentialOutStream, 0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00);
MY_DEFINE_GUID(IID_IInStream, 0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00);
MY_DEFINE_GUID(IID_IOutStream, 0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00);
MY_DEFINE_GUID(IID_IArchiveExtractCallback, 0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x06, 0x00, 0x20, 0x00, 0x00);
MY_DEFINE_GUID(IID_IArchiveOpenCallback, 0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00);
MY_DEFINE_GUID(IID_IArchiveOpenVolumeCallback, 0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x06, 0x00, 0x30, 0x00, 0x00);
MY_DEFINE_GUID(IID_IArchiveUpdateCallback, 0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x06, 0x00, 0x80, 0x00, 0x00);
MY_DEFINE_GUID(IID_IArchiveUpdateCallback2, 0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x06, 0x00, 0x82, 0x00, 0x00);
MY_DEFINE_GUID(IID_ICryptoGetTextPassword, 0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00);
MY_DEFINE_GUID(IID_ICryptoGetTextPassword2, 0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x05, 0x00, 0x11, 0x00, 0x00);
