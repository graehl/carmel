import tree
import re

rule_arrow_s='->'
#rule_arrow=re.compile(r"\s*\-\>\s*")
rule_bangs_s=r"###"
rhs_word_s=r'\S+'
rhs_word=re.compile(rhs_word_s)
#rule_bangs=re.compile(rule_bangs_s)
#featval=r"{{{.*?}}}|\S+"
#featname=r"[^ =]+"
#rule_feat=re.compile(r'\s*('+featname+')=('+featval+')')
rule_id_s=r"\bid=(-?\d+)\b"
rule_id=re.compile(rule_id_s)
find_id_whole_rule=re.compile(rule_bangs_s+".*"+rule_id_s)
lhs_token_s=r'\(|\)|[^\s"()]+|"""|""""|"[^"\s]*"' #DANGER: does this try/backtrack left->right? if so, ok. otherwise, problems?
lhs_tokens=re.compile(lhs_token_s)
space_re=re.compile(r'\s*')

def find_rule_id(s,iattr=0):
    m=rule_id.search(s,iattr)
    if not m:
        raise Exception("couldn't find id= in rule attributes %s"%s[iattr:])
    return m.group(1)

def tokenize(re,s,until_token=None,pos=0,allow_space=True):
    'return parallel lists [token],[(i,j)] so that s[i:j]=token. if allow_space is False, then the entire string must be consumed. otherwise spaces anywhere between tokens are ignored. until_token is a terminator (which is included in the return)'
    tokens=[]
    spans=[]
    while True:
        if allow_space:
            m=space_re.match(s,pos)
            if m: pos=m.end()
        if pos==len(s):
            break
        m=re.match(s,pos)
        if not m:
            raise Exception("tokenize: re %s didn't match string %s at position %s - remainder %s"%(re,s,pos,s[pos:]))
        pos2=m.end()
        t=s[pos:pos2]
        tokens.append(t)
        spans.append((pos,pos2))
        if t==until_token:
            break
        pos=pos2
    return (tokens,spans)

    #sep=False
    # for x in re.split(s):
    #     if sep: r.append(x)
    #     elif allow_space:
    #         if not space_re.match(x): return None
    #     else:
    #         if len(x): return None
    #     sep=not sep
    # return r


# return (tree,start-rhs-pos,end-tree-pos)
def parse_sbmt_lhs(s,require_arrow=True):
    (tokens,spans)=tokenize(lhs_tokens,s,rule_arrow_s)
    if not len(tokens):
        raise Exception("sbmt rule has no LHS tokens: %s"%s)
    (_,p2)=spans[-1]
    if tokens[-1] == rule_arrow_s:
        tokens.pop()
    elif require_arrow:
        raise Exception("sbmt rule LHS not terminated in %s: %s"%(rule_arrow_s,tokens))
    (t,endt)=tree.scan_tree(tokens,0,True)
    if t is None or endt != len(tokens):
        raise Exception("sbmt rule LHS tokens weren't parsed into a tree: %s TREE_ENDS_HERE unparsed = %s"%(tokens[0:endt],tokens[endt:]))
    (_,p1)=spans[endt]
    return (t,p2,p1)

# return (tree,[rhs-tokens],start-attr-pos,end-rhs-pos,start-rhs-pos,end-tree-pos). s[start-attr-pos:] are the unparsed attributes (if any) for the rule
def parse_sbmt_rule(s,require_arrow=True,require_bangs=True): #bangs = the ### sep. maybe you want to parse rule w/o any features.
    (t,p2,p1)=parse_sbmt_lhs(s,require_arrow)
    (rhs,rspans)=tokenize(rhs_word,s,rule_bangs_s,p2)
    if len(rhs):
        (_,p4)=rspans[-1]
        if rhs[-1]==rule_bangs_s:
            rhs.pop()
            (_,p3)=rspans[-2]
        elif require_bangs:
            raise Exception("sbmt rule RHS wasn't terminated in %s: %s"%(rule_bangs_s,rhs))
        else:
            p3=p4
    else:
        p3=p2
        p4=p2
#    dumpx(str(t),rhs,p1,p2,p3,s[p3:])
    return (t,rhs,p4,p3,p2,p1)


