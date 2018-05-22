#ifndef LIBSIMPLIFY_EPWING_HOOK_HH_
#define LIBSIMPLIFY_EPWING_HOOK_HH_

#include <eb/eb.h>

namespace simplify {

struct HookContext {
    struct ByteRange {
        Charset charset;
        size_t offset;
        size_t length;
    };

    std::vector<ByteRange> byte_ranges;
    size_t buffer_length;
    v8::Persistent<v8::Function> *js_functions;
};

static EB_Error_Code HandleIso8859_1(EB_Book *, EB_Appendix *, void *,
                                     EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleJisX0208(EB_Book *, EB_Appendix *, void *,
                                    EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleGb2312(EB_Book *, EB_Appendix *, void *,
                                  EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleBeginSub(EB_Book *, EB_Appendix *, void *,
                                    EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleEndSub(EB_Book *, EB_Appendix *, void *,
                                  EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleBeginSup(EB_Book *, EB_Appendix *, void *,
                                    EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleEndSup(EB_Book *, EB_Appendix *, void *,
                                  EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleIndent(EB_Book *, EB_Appendix *, void *,
                                  EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleNewline(EB_Book *, EB_Appendix *, void *,
                                   EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleBeginNoBr(EB_Book *, EB_Appendix *, void *,
                                     EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleEndNoBr(EB_Book *, EB_Appendix *, void *,
                                   EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleBeginEm(EB_Book *, EB_Appendix *, void *,
                                   EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleEndEm(EB_Book *, EB_Appendix *, void *,
                                 EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleBeginReference(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleEndReference(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleBeginKeyword(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleEndKeyword(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleBeginDecoration(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleEndDecoration(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleInsertHGaiji(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleInsertTGaiji(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *); 

}  // namespace simplify

#endif
