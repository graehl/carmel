import tree
import re
from graehl import warn

radu_drophead=re.compile(r'\(([^~]+)~(\d+)~(\d+)\s+(-?[.0123456789]+)')
radu_keephead=re.compile(r'\((\S+)\s+(-?[.0123456789]+)')
radu_keephead=re.compile(r'\(([^~]+~\d+~\d+)\s+(-?[.0123456789]+)')
sym_rrb=re.compile(r'\((\S+) (\S+)\)')
rparen=re.compile(r'\)')
lparen=re.compile(r'\(')
def escape_paren(s):
    s=rparen.sub('-RRB-',s)
    return lparen.sub('-LRB-',s)
def rrb_repl(match):
    return '(%s %s)'%(match.group(1),escape_paren(match.group(2)))
def escape_rrb(t):
    return sym_rrb.sub(rrb_repl,t)
def radu2ptb(t,strip_head=True):
    "radu format: all close parens that indicate tree structure are followed by space or end, so that () are legal within symbols -   also, we strip head info.  we escape them to -LRB- -RRB- for handling by tree.py."
    t=(radu_drophead if strip_head else radu_keephead).sub(r'(\1',t)
    t=escape_rrb(t)
    return t

def str_to_tree_warn(s,paren_after_root=False,max=None):
    toks=tree.tokenizer.findall(s)
    if len(toks)>2 and toks[0] == '(' and toks[1]=='(' and toks[-2]==')' and toks[-1]==')': #berkeley parse ( (tree) )
        toks=toks[1:-1]
    (t,n)=tree.scan_tree(toks,0,paren_after_root)
    if t is None:
        warn("scan_tree failed",": %s of %s: %s ***HERE*** %s"%(n,len(toks),' '.join(toks[:n]),' '.join(toks[n:])),max=max)
    return t


headindex_re=re.compile(r'([^~]+)~(\d+)~(\d+)')
raduhead_base_skips=set(['.',"''",'``'])
raduhead_more_skips=set([':',','])
raduhead_skips=raduhead_base_skips.union(raduhead_more_skips)
raduhead_npb_skips=raduhead_base_skips
#,'NML','ADJP'
def raduhead(t,dbgmsg='',headword=True):
    m=headindex_re.match(t.label)
    t.label_orig=t.label
    if m is None:
        if headword and t.is_preterminal():
            t.headword=(t.label,t.children[0].label)
        return
    label,n,head=m.group(1,2,3)
    skips=(raduhead_npb_skips if label=='NPB' else raduhead_skips)
    #warn('raduhead','%s=%s,%s,%s'%(t.label,label,head,n))
    t.head=head=int(head)
    n=int(n)
    i=0
    #i==0 and c.label==':' or
    cn=[]
    for c in t.children:
        if c.label not in skips:
            cn.append(c)
        i+=1
    t.head_children=cn
    t.good_head=(n==len(cn) and head<=n and  head>0)
    if not t.good_head:
        warn('wrong head index for %s'%label,('%s!=%s %s => %s %s')%(n,len(cn),t.label,[c.label_orig for c in t.children],dbgmsg),max=None)
        if headword: t.headword=t.head_children[-1].headword
    elif headword:
        t.headword=t.head_children[head-1].headword
    t.label=label

def raduparse(tline,intern_labels=False,strip_head=True):
    t=radu2ptb(tline,strip_head=strip_head)
    t=str_to_tree_warn(t)
    if intern_labels:
        t.mapinplace(intern)
    if not strip_head:
        #warn('raduparse','%s=%s'%(tline,t.str(square=True)))
        for n in t.postorder():
            raduhead(n,dbgmsg=tline)
    return t

def is_bar(s):
    return len(s)>5 and s[0]=='@' and s[-4:]=='-BAR'

def strip_bar(s):
    if len(s)<5: return s
    if s[0]=='@' and s[-4:]=='-BAR': return s[1:-4]
    return s

def no_bar(s):
    return None if is_bar(s) else s

subcat_s=r'-\d+(?=$|-BAR)'
subcat_re=re.compile(subcat_s)
def is_subcat(s):
    return subcat_re.match(s)

def strip_subcat(s):
    return subcat_re.sub('',s)
