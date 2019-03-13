#include "fitz-internal.h"
#include "mupdf-internal.h"
#include "Windows.h"
#define TILE
//#define debug

typedef struct pdf_material_s pdf_material;
typedef struct pdf_gstate_s pdf_gstate;
typedef struct pdf_csi_s pdf_csi;

struct spointstack cstack;//���ջָ���һ��ջ�ṹ ��ʼ�� ʹ�� pop push delete
struct zblrouteset *getline;
struct zblstack* currentstackpoint;
float currentlinewidth=1;//�����ǰ��ɫ�ռ�,��ɫ,�߿��˳������
float Vpointx,Vpointy;
int colorchanged=0;//Ĭ��Ϊ�� 1-��ɫ 2-���
int currentfillcolorspace;
int currentstrokecolorspace;
float currentfillcolor[4];//��ǰ��ɫ ���·��
float currentstrokecolor[4];//��ǰ��ɫ fill���
int stackstate;//�ж��Ƿ���ͼ��ջ����

void initcstack()//��ʼ��ջ
{
	struct zstacknode* node=(struct zstacknode*)malloc(sizeof(struct zstacknode));
	node->pointer=NULL;
	node->next=NULL;
	cstack.head=node;
	cstack.tail=cstack.head;
}

void push(struct zblstack* pointer)//��ջ����
{
	struct zstacknode* node=(struct zstacknode*)malloc(sizeof(struct zstacknode));
	node->pointer=pointer;
	node->next=cstack.head;
	cstack.head=node;
}

void pop()//��ջ����
{
	if(cstack.head==cstack.tail)
	{
		printf("ջΪ��\n");
	}
	else
	{
		struct zstacknode* node=cstack.head;
		cstack.head=node->next;
		free(node);
	}
}

struct zblstack* currentstack()//��ȡջ���ڵ��ŵ�ָ��
{
	return cstack.head->pointer;
}

void initpoint(struct routepoint* head)
{
	head->p0=-1;
	head->p1=-1;
	head->p2=-1;
	head->p3=-1;
	head->p4=-1;
	head->p5=-1;
	head->state=-1;
	head->nextpoint=NULL;
}

void initroute(struct zblroute* route)
{
	//��ʼ������
	route->type=100;
	route->linewidth=-1;
	route->colorspace=-1;
	route->color[0]=0;
	route->color[1]=0;
	route->color[2]=0;
	route->color[3]=0;
	route->scolorspace=0;
	route->scolor=NULL;
	route->countpoint=0;
	route->drawingmethord=0;
	//����ͷָ��ռ�
	route->pointheadler=(struct routepoint*)malloc(sizeof(struct routepoint));
	route->currentpoint=route->pointheadler;
	initpoint(route->pointheadler);
	route->nextroute=NULL;
}

void initstack(struct zblstack* stack)
{
	stack->existstack=0;//Ĭ��0��ʾ���ͼ�ζ�����ջ��
	stack->countroute=0;//��¼һ��ͼ��״̬ջ���ж��ٸ�·��
	//Ĭ�ϲ����ڼ���·��
	stack->existclip=0;
	stack->countclip=0;

	stack->existnest=0;//Ĭ�ϲ�����Ƕ��
	stack->clipheadler=NULL;
	stack->currentclip=NULL;
	//Ĭ�ϲ�����ת�þ���
	stack->existcm=0;
	stack->matrix=NULL;
	//������ɫ
	stack->shadowtype=0;//Ĭ�ϲ����ڽ���ɫ
	stack->bcolor=NULL;
	stack->ecolor=NULL;
	stack->ca=1;

	stack->stackheadler=(struct zblstack*)malloc(sizeof (struct zblstack));
	stack->currentstack=stack->stackheadler;
	stack->routeheadler=(struct zblroute*)malloc(sizeof(struct zblroute));
	stack->currentroute=stack->routeheadler;
	initroute(stack->routeheadler);
	stack->nextstack=NULL;
}

void initrouteset(struct zblrouteset* getline)
{
	getline->count=0;
	getline->stackheadler=(struct zblstack*)malloc(sizeof(struct zblstack));
	getline->currentstack=getline->stackheadler;
	initstack(getline->stackheadler);
}
void addpoint(struct zblroute* route)
{
	struct routepoint* point=(struct routepoint*)malloc(sizeof(struct routepoint));
	point->p0=-1;
	point->p1=-1;
	point->p2=-1;
	point->p3=-1;
	point->p4=-1;
	point->p5=-1;
	point->state=-1;
	point->nextpoint=NULL;
	route->currentpoint->nextpoint=point;
	route->currentpoint=route->currentpoint->nextpoint;
}

void addroute(struct zblstack* stack)
{
	struct zblroute* route=(struct zblroute*)malloc(sizeof(struct zblroute));
	//��ʼ������
	route->type=0;
	route->linewidth=-1;
	route->colorspace=-1;
	route->color[0]=0;
	route->color[1]=0;
	route->color[2]=0;
	route->color[3]=0;
	route->scolorspace=0;
	route->scolor=NULL;
	route->countpoint=0;
	route->drawingmethord=0;
	//����ͷָ��ռ�
	route->pointheadler=(struct routepoint*)malloc(sizeof(struct routepoint));
	route->currentpoint=route->pointheadler;
	route->nextroute=NULL;
	addpoint(route);
	stack->currentroute->nextroute=route;
	stack->currentroute=stack->currentroute->nextroute;
}

void addneststack(struct zblstack* pointer)//ΪջǶ�����������ջ
{
	struct zblstack* stack=(struct zblstack*)malloc(sizeof(struct zblstack));
	stack->existstack=0;//Ĭ��0��ʾ���ͼ�ζ�����ջ��
	stack->countroute=0;
	//Ĭ�ϲ����ڼ���·��
	stack->existclip=0;
	stack->countclip=0;

	stack->existnest=0;
	stack->clipheadler=NULL;
	stack->currentclip=NULL;
	//Ĭ�ϲ�����ת�þ���
	stack->existcm=0;
	stack->matrix=NULL;

	//������ɫ
	stack->shadowtype=0;//Ĭ�ϲ����ڽ���ɫ
	stack->bcolor=NULL;
	stack->ecolor=NULL;
	stack->ca=1;

	stack->routeheadler=(struct zblroute*)malloc(sizeof(struct zblroute));
	stack->currentroute=stack->routeheadler;
	stack->nextstack=NULL;

	initstack(stack);

	addroute(stack);//���ջ
	//pointer->stackheadler=(struct zblstack*)malloc(sizeof(struct zblstack));
	pointer->currentstack->nextstack=stack;
	pointer->currentstack=pointer->currentstack->nextstack;
}


void addstack(struct zblrouteset* getline)
{
	struct zblstack* stack=(struct zblstack*)malloc(sizeof(struct zblstack));
	stack->existstack=0;//Ĭ��0��ʾ���ͼ�ζ�����ջ��
	stack->countroute=0;
	//Ĭ�ϲ����ڼ���·��
	stack->existclip=0;
	stack->countclip=0;
	stack->existstack=0;
	stack->existnest=0;
	//������ɫ
	stack->shadowtype=0;//Ĭ�ϲ����ڽ���ɫ
	stack->bcolor=NULL;
	stack->ecolor=NULL;
	stack->ca=1;
	stack->clipheadler=NULL;
	stack->currentclip=NULL;
	//Ĭ�ϲ�����ת�þ���
	stack->existcm=0;
	stack->matrix=NULL;

	stack->stackheadler=(struct zblstack*)malloc(sizeof (struct zblstack));
	stack->currentstack=stack->stackheadler;
	stack->routeheadler=(struct zblroute*)malloc(sizeof(struct zblroute));
	stack->currentroute=stack->routeheadler;
	stack->nextstack=NULL;
	addroute(stack);//���ջ
	getline->currentstack->nextstack=stack;
	getline->currentstack=getline->currentstack->nextstack;
}

void addclippoint(struct zblstack* stack)
{
	struct routepoint* point=(struct routepoint*)malloc(sizeof(struct routepoint));
	point->p0=-1;
	point->p1=-1;
	point->p2=-1;
	point->p3=-1;
	point->p4=-1;
	point->p5=-1;
	point->state=-1;
	point->nextpoint=NULL;
	stack->currentclip->nextpoint=point;
	stack->currentclip=stack->currentclip->nextpoint;
}

void copyclippoint(struct zblstack* stack)
{
	stack->currentclip->p0=stack->currentroute->currentpoint->p0;
	stack->currentclip->p1=stack->currentroute->currentpoint->p1;
	stack->currentclip->p2=stack->currentroute->currentpoint->p2;
	stack->currentclip->p3=stack->currentroute->currentpoint->p3;
	stack->currentclip->p4=stack->currentroute->currentpoint->p4;
	stack->currentclip->p5=stack->currentroute->currentpoint->p5;
	stack->currentclip->state=stack->currentroute->currentpoint->state;
}

void copycliproute(struct zblstack* stack)
{
	struct zblroute* route=NULL;
	route=stack->currentroute;//��ǰջָ��
	stack->countroute=0;
	stack->countclip=stack->currentroute->countpoint;
	
	stack->currentroute->currentpoint=stack->currentroute->pointheadler;
	stack->clipheadler=(struct routepoint*)malloc(sizeof(struct routepoint));//���䵱ǰ����·��ָ��
	stack->currentclip=stack->clipheadler;
	initpoint(stack->clipheadler);
	while(stack->currentroute->currentpoint->nextpoint!=NULL)
	{
		stack->currentroute->currentpoint=stack->currentroute->currentpoint->nextpoint;
		addclippoint(stack);//��ӵ㲢��ʼ��
		copyclippoint(stack);//���и���
	}
	/*
	free(stack->routeheadler);
	free(stack->currentroute);
	stack->routeheadler=(struct zblroute*)malloc(sizeof(struct zblroute));//��ʼ��route
	stack->currentroute=stack->routeheadler;
	initroute(stack->routeheadler);
	*/
	free(stack->currentroute->pointheadler);
	free(stack->currentroute->currentpoint);
	stack->currentroute->countpoint=0;
	stack->currentroute->pointheadler=(struct routepoint*)malloc(sizeof(struct routepoint));
	stack->currentroute->currentpoint=stack->currentroute->pointheadler;
	initpoint(stack->currentroute->pointheadler);
	addpoint(stack->currentroute);
}

enum
{
	PDF_FILL,
	PDF_STROKE,
};

enum
{
	PDF_MAT_NONE,
	PDF_MAT_COLOR,
	PDF_MAT_PATTERN,
	PDF_MAT_SHADE,
};

struct pdf_material_s
{
	int kind;
	fz_colorspace *colorspace;
	pdf_pattern *pattern;
	fz_shade *shade;
	float alpha;
	float v[32];
};

struct pdf_gstate_s
{
	fz_matrix ctm;
	int clip_depth;

	/* path stroking */
	fz_stroke_state *stroke_state;

	/* materials ����*/
	pdf_material stroke;//��
	pdf_material fill;

	/* text state �ı�״̬*/
	float char_space;
	float word_space;
	float scale;
	float leading;
	pdf_font_desc *font;
	float size;
	int render;
	float rise;

	/* transparency ͸����*/
	int blendmode;
	pdf_xobject *softmask;
	fz_matrix softmask_ctm;
	float softmask_bc[FZ_MAX_COLORS];
	int luminosity;
};

struct pdf_csi_s
{
	fz_device *dev;
	pdf_document *xref;

	/* usage mode for optional content groups ��ѡ�������ʹ��ģʽ */
	char *event; /* "View", "Print", "Export" */

	/* interpreter stack ����ջ*/
	pdf_obj *obj;
	char name[256];
	unsigned char string[256];
	int string_len;
	float stack[32];
	int top;

	int xbalance;
	int in_text;
	int in_hidden_ocg;

	/* path object state ·�������״̬*/
	fz_path *path;
	int clip;
	int clip_even_odd;

	/* text object state �ı������״̬*/
	fz_text *text;
	fz_rect text_bbox;
	fz_matrix tlm;
	fz_matrix tm;
	int text_mode;
	int accumulate;

	/* graphics state ͼ��״̬*/
	fz_matrix top_ctm;//3*3��ctm������������任
	pdf_gstate *gstate;
	int gcap;
	int gtop;

	/* cookie support */
	fz_cookie *cookie;
};

static void pdf_run_buffer(pdf_csi *csi, pdf_obj *rdb, fz_buffer *contents);
static void pdf_run_xobject(pdf_csi *csi, pdf_obj *resources, pdf_xobject *xobj, fz_matrix transform);
static void pdf_show_pattern(pdf_csi *csi, pdf_pattern *pat, fz_rect area, int what);

static int
ocg_intents_include(pdf_ocg_descriptor *desc, char *name)
{
	int i, len;

	if (strcmp(name, "All") == 0)
		return 1;

	/* In the absence of a specified intent, it's 'View' */
	if (!desc->intent)
		return (strcmp(name, "View") == 0);

	if (pdf_is_name(desc->intent))
	{
		char *intent = pdf_to_name(desc->intent);
		if (strcmp(intent, "All") == 0)
			return 1;
		return (strcmp(intent, name) == 0);
	}
	if (!pdf_is_array(desc->intent))
		return 0;

	len = pdf_array_len(desc->intent);
	for (i=0; i < len; i++)
	{
		char *intent = pdf_to_name(pdf_array_get(desc->intent, i));
		if (strcmp(intent, "All") == 0)
			return 1;
		if (strcmp(intent, name) == 0)
			return 1;
	}
	return 0;
}

static int
pdf_is_hidden_ocg(pdf_obj *ocg, pdf_csi *csi, pdf_obj *rdb)
{
	char event_state[16];
	pdf_obj *obj, *obj2;
	char *type;
	pdf_ocg_descriptor *desc = csi->xref->ocg;

	/* If no ocg descriptor, everything is visible */
	if (!desc)
		return 0;

	/* If we've been handed a name, look it up in the properties. */
	if (pdf_is_name(ocg))
	{
		ocg = pdf_dict_gets(pdf_dict_gets(rdb, "Properties"), pdf_to_name(ocg));
	}
	/* If we haven't been given an ocg at all, then we're visible */
	if (!ocg)
		return 0;

	fz_strlcpy(event_state, csi->event, sizeof event_state);
	fz_strlcat(event_state, "State", sizeof event_state);

	type = pdf_to_name(pdf_dict_gets(ocg, "Type"));

	if (strcmp(type, "OCG") == 0)
	{
		/* An Optional Content Group */
		int num = pdf_to_num(ocg);
		int gen = pdf_to_gen(ocg);
		int len = desc->len;
		int i;

		for (i = 0; i < len; i++)
		{
			if (desc->ocgs[i].num == num && desc->ocgs[i].gen == gen)
			{
				if (desc->ocgs[i].state == 0)
					return 1; /* If off, hidden */
				break;
			}
		}

		/* Check Intents; if our intent is not part of the set given
		 * by the current config, we should ignore it. */
		obj = pdf_dict_gets(ocg, "Intent");
		if (pdf_is_name(obj))
		{
			/* If it doesn't match, it's hidden */
			if (ocg_intents_include(desc, pdf_to_name(obj)) == 0)
				return 1;
		}
		else if (pdf_is_array(obj))
		{
			int match = 0;
			len = pdf_array_len(obj);
			for (i=0; i<len; i++) {
				match |= ocg_intents_include(desc, pdf_to_name(pdf_array_get(obj, i)));
				if (match)
					break;
			}
			/* If we don't match any, it's hidden */
			if (match == 0)
				return 1;
		}
		else
		{
			/* If it doesn't match, it's hidden */
			if (ocg_intents_include(desc, "View") == 0)
				return 1;
		}

		/* FIXME: Currently we do a very simple check whereby we look
		 * at the Usage object (an Optional Content Usage Dictionary)
		 * and check to see if the corresponding 'event' key is on
		 * or off.
		 *
		 * Really we should only look at Usage dictionaries that
		 * correspond to entries in the AS list in the OCG config.
		 * Given that we don't handle Zoom or User, or Language
		 * dicts, this is not really a problem. */
		obj = pdf_dict_gets(ocg, "Usage");
		if (!pdf_is_dict(obj))
			return 0;
		/* FIXME: Should look at Zoom (and return hidden if out of
		 * max/min range) */
		/* FIXME: Could provide hooks to the caller to check if
		 * User is appropriate - if not return hidden. */
		obj2 = pdf_dict_gets(obj, csi->event);
		if (strcmp(pdf_to_name(pdf_dict_gets(obj2, event_state)), "OFF") == 0)
		{
			return 1;
		}
		return 0;
	}
	else if (strcmp(type, "OCMD") == 0)
	{
		/* An Optional Content Membership Dictionary */
		char *name;
		int combine, on;

		obj = pdf_dict_gets(ocg, "VE");
		if (pdf_is_array(obj)) {
			/* FIXME: Calculate visibility from array */
			return 0;
		}
		name = pdf_to_name(pdf_dict_gets(ocg, "P"));
		/* Set combine; Bit 0 set => AND, Bit 1 set => true means
		 * Off, otherwise true means On */
		if (strcmp(name, "AllOn") == 0)
		{
			combine = 1;
		}
		else if (strcmp(name, "AnyOff") == 0)
		{
			combine = 2;
		}
		else if (strcmp(name, "AllOff") == 0)
		{
			combine = 3;
		}
		else	/* Assume it's the default (AnyOn) */
		{
			combine = 0;
		}

		obj = pdf_dict_gets(ocg, "OCGs");
		on = combine & 1;
		if (pdf_is_array(obj)) {
			int i, len;
			len = pdf_array_len(obj);
			for (i = 0; i < len; i++)
			{
				int hidden;
				hidden = pdf_is_hidden_ocg(pdf_array_get(obj, i), csi, rdb);
				if ((combine & 1) == 0)
					hidden = !hidden;
				if (combine & 2)
					on &= hidden;
				else
					on |= hidden;
			}
		}
		else
		{
			on = pdf_is_hidden_ocg(obj, csi, rdb);
			if ((combine & 1) == 0)
				on = !on;
		}
		return !on;
	}
	/* No idea what sort of object this is - be visible */
	return 0;
}

/*
 * Emit graphics calls to device.�����豸����ͼ��
 */

static void
pdf_begin_group(pdf_csi *csi, fz_rect bbox)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;

	if (gstate->softmask)
	{
		pdf_xobject *softmask = gstate->softmask;
		fz_rect bbox = fz_transform_rect(gstate->softmask_ctm, softmask->bbox);
		fz_matrix save_ctm = gstate->ctm;

		gstate->softmask = NULL;
		gstate->ctm = gstate->softmask_ctm;

		fz_begin_mask(csi->dev, bbox, gstate->luminosity,
			softmask->colorspace, gstate->softmask_bc);
		pdf_run_xobject(csi, NULL, softmask, fz_identity);
		/* RJW: "cannot run softmask" */
		fz_end_mask(csi->dev);

		gstate->softmask = softmask;
		gstate->ctm = save_ctm;
	}

	if (gstate->blendmode)
		fz_begin_group(csi->dev, bbox, 1, 0, gstate->blendmode, 1);
}

static void
pdf_end_group(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;

	if (gstate->blendmode)
		fz_end_group(csi->dev);

	if (gstate->softmask)
		fz_pop_clip(csi->dev);
}

static void
pdf_show_shade(pdf_csi *csi, fz_shade *shd)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	fz_rect bbox;

	if (csi->in_hidden_ocg > 0)
		return;

	bbox = fz_bound_shade(ctx, shd, gstate->ctm);

	pdf_begin_group(csi, bbox);

	fz_fill_shade(csi->dev, shd, gstate->ctm, gstate->fill.alpha);

	pdf_end_group(csi);
}

static void
pdf_show_image(pdf_csi *csi, fz_image *image)//��ʾͼƬ
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	fz_matrix image_ctm;
	fz_rect bbox;

	if (csi->in_hidden_ocg > 0)
		return;

	/* PDF has images bottom-up, so flip them right side up here PDF�����¶��ϵ�ͼ�����Խ����Ƿ�ת����*/
	image_ctm = fz_concat(fz_scale(1, -1), fz_translate(0, 1));
	image_ctm = fz_concat(image_ctm, gstate->ctm);

	bbox = fz_transform_rect(image_ctm, fz_unit_rect);

	if (image->mask)
	{
		/* apply blend group even though we skip the softmask ʹ�û���鼴ʹ����������softmask*/
		if (gstate->blendmode)
			fz_begin_group(csi->dev, bbox, 0, 0, gstate->blendmode, 1);
		fz_clip_image_mask(csi->dev, image->mask, &bbox, image_ctm);
	}
	else
		pdf_begin_group(csi, bbox);

	if (!image->colorspace)
	{

		switch (gstate->fill.kind)
		{
		case PDF_MAT_NONE:
			break;
		case PDF_MAT_COLOR:
			fz_fill_image_mask(csi->dev, image, image_ctm,
				gstate->fill.colorspace, gstate->fill.v, gstate->fill.alpha);
			break;
		case PDF_MAT_PATTERN:
			if (gstate->fill.pattern)
			{
				fz_clip_image_mask(csi->dev, image, &bbox, image_ctm);
				pdf_show_pattern(csi, gstate->fill.pattern, bbox, PDF_FILL);
				fz_pop_clip(csi->dev);
			}
			break;
		case PDF_MAT_SHADE:
			if (gstate->fill.shade)
			{
				fz_clip_image_mask(csi->dev, image, &bbox, image_ctm);
				fz_fill_shade(csi->dev, gstate->fill.shade, gstate->ctm, gstate->fill.alpha);
				fz_pop_clip(csi->dev);
			}
			break;
		}
	}
	else
	{
		fz_fill_image(csi->dev, image, image_ctm, gstate->fill.alpha);
	}

	if (image->mask)
	{
		fz_pop_clip(csi->dev);
		if (gstate->blendmode)
			fz_end_group(csi->dev);
	}
	else
		pdf_end_group(csi);
}

static void
pdf_show_path(pdf_csi *csi, int doclose, int dofill, int dostroke, int even_odd)//����·�������ĸ������жϲ������ͣ���պϣ���䣬����even_odd(?)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	fz_path *path;
	fz_rect bbox;

	if (dostroke) {
		if (csi->dev->flags & (FZ_DEVFLAG_STROKECOLOR_UNDEFINED | FZ_DEVFLAG_LINEJOIN_UNDEFINED | FZ_DEVFLAG_LINEWIDTH_UNDEFINED))
			csi->dev->flags |= FZ_DEVFLAG_UNCACHEABLE;
		else if (gstate->stroke_state->dash_len != 0 && csi->dev->flags & (FZ_DEVFLAG_STARTCAP_UNDEFINED | FZ_DEVFLAG_DASHCAP_UNDEFINED | FZ_DEVFLAG_ENDCAP_UNDEFINED))
			csi->dev->flags |= FZ_DEVFLAG_UNCACHEABLE;
		else if (gstate->stroke_state->linejoin == FZ_LINEJOIN_MITER && (csi->dev->flags & FZ_DEVFLAG_MITERLIMIT_UNDEFINED))
			csi->dev->flags |= FZ_DEVFLAG_UNCACHEABLE;
	}
	if (dofill) {
		if (csi->dev->flags & FZ_DEVFLAG_FILLCOLOR_UNDEFINED)
			csi->dev->flags |= FZ_DEVFLAG_UNCACHEABLE;
	}

	path = csi->path;//��csi->path���ݸ��������,������csi->path
	csi->path = fz_new_path(ctx);

	fz_try(ctx)
	{
		if (doclose)//����·��
			fz_closepath(ctx, path);

		if (dostroke)//?
			bbox = fz_bound_path(ctx, path, gstate->stroke_state, gstate->ctm);
		else
			bbox = fz_bound_path(ctx, path, NULL, gstate->ctm);

		if (csi->clip)
		{
			gstate->clip_depth++;
			fz_clip_path(csi->dev, path, NULL, csi->clip_even_odd, gstate->ctm);
			csi->clip = 0;
		}

		if (csi->in_hidden_ocg > 0)
			dostroke = dofill = 0;

		if (dofill || dostroke)//����?
			pdf_begin_group(csi, bbox);

		if (dofill)
		{
			switch (gstate->fill.kind)//�������
			{
			case PDF_MAT_NONE:
				break;
			case PDF_MAT_COLOR:
				// cf. http://code.google.com/p/sumatrapdf/issues/detail?id=966
				if (6 <= path->len && path->len <= 7 && path->items[0].k == FZ_MOVETO && path->items[3].k == FZ_LINETO &&
					(path->items[1].v != path->items[4].v || path->items[2].v != path->items[5].v))
				{
					fz_stroke_state *stroke = fz_new_stroke_state(ctx);
					stroke->linewidth = 0.1f / fz_matrix_expansion(gstate->ctm);
					fz_stroke_path(csi->dev, path, stroke, gstate->ctm,
						gstate->fill.colorspace, gstate->fill.v, gstate->fill.alpha);
					fz_drop_stroke_state(ctx, stroke);
					break;
				}
				fz_fill_path(csi->dev, path, even_odd, gstate->ctm,
					gstate->fill.colorspace, gstate->fill.v, gstate->fill.alpha);//���·������
				break;
			case PDF_MAT_PATTERN:
				if (gstate->fill.pattern)
				{
					fz_clip_path(csi->dev, path, NULL, even_odd, gstate->ctm);
					pdf_show_pattern(csi, gstate->fill.pattern, bbox, PDF_FILL);
					fz_pop_clip(csi->dev);
				}
				break;
			case PDF_MAT_SHADE:
				if (gstate->fill.shade)
				{
					fz_clip_path(csi->dev, path, NULL, even_odd, gstate->ctm);
					fz_fill_shade(csi->dev, gstate->fill.shade, csi->top_ctm, gstate->fill.alpha);
					fz_pop_clip(csi->dev);
				}
				break;
			}
		}

		if (dostroke)
		{
			switch (gstate->stroke.kind)
			{
			case PDF_MAT_NONE:
				break;
			case PDF_MAT_COLOR:
				fz_stroke_path(csi->dev, path, gstate->stroke_state, gstate->ctm,
					gstate->stroke.colorspace, gstate->stroke.v, gstate->stroke.alpha);
				break;
			case PDF_MAT_PATTERN:
				if (gstate->stroke.pattern)
				{
					fz_clip_stroke_path(csi->dev, path, &bbox, gstate->stroke_state, gstate->ctm);
					pdf_show_pattern(csi, gstate->stroke.pattern, bbox, PDF_STROKE);
					fz_pop_clip(csi->dev);
				}
				break;
			case PDF_MAT_SHADE:
				if (gstate->stroke.shade)
				{
					fz_clip_stroke_path(csi->dev, path, &bbox, gstate->stroke_state, gstate->ctm);
					fz_fill_shade(csi->dev, gstate->stroke.shade, csi->top_ctm, gstate->stroke.alpha);
					fz_pop_clip(csi->dev);
				}
				break;
			}
		}

		if (dofill || dostroke)
			pdf_end_group(csi);
	}
	fz_catch(ctx)
	{
		fz_free_path(ctx, path);
		fz_rethrow(ctx);
	}
	fz_free_path(ctx, path);
}

/*
 * Assemble and emit text
   ��������ı�
 */

static void
pdf_flush_text(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	fz_text *text;
	int dofill = 0;
	int dostroke = 0;
	int doclip = 0;
	int doinvisible = 0;
	fz_context *ctx = csi->dev->ctx;

	if (!csi->text)
		return;
	text = csi->text;
	csi->text = NULL;

	dofill = dostroke = doclip = doinvisible = 0;
	switch (csi->text_mode)
	{
	case 0: dofill = 1; break;
	case 1: dostroke = 1; break;
	case 2: dofill = dostroke = 1; break;
	case 3: doinvisible = 1; break;
	case 4: dofill = doclip = 1; break;
	case 5: dostroke = doclip = 1; break;
	case 6: dofill = dostroke = doclip = 1; break;
	case 7: doclip = 1; break;
	}

	if (csi->in_hidden_ocg > 0)
		dostroke = dofill = 0;

	fz_try(ctx)
	{
		pdf_begin_group(csi, csi->text_bbox);

		if (doinvisible)
			fz_ignore_text(csi->dev, text, gstate->ctm);

		if (doclip)
		{
			if (csi->accumulate < 2)
				gstate->clip_depth++;
			fz_clip_text(csi->dev, text, gstate->ctm, csi->accumulate);
			csi->accumulate = 2;
		}

		if (dofill)
		{
			switch (gstate->fill.kind)
			{
			case PDF_MAT_NONE:
				break;
			case PDF_MAT_COLOR:
				fz_fill_text(csi->dev, text, gstate->ctm,
					gstate->fill.colorspace, gstate->fill.v, gstate->fill.alpha);
				break;
			case PDF_MAT_PATTERN:
				if (gstate->fill.pattern)
				{
					fz_clip_text(csi->dev, text, gstate->ctm, 0);
					pdf_show_pattern(csi, gstate->fill.pattern, csi->text_bbox, PDF_FILL);
					fz_pop_clip(csi->dev);
				}
				break;
			case PDF_MAT_SHADE:
				if (gstate->fill.shade)
				{
					fz_clip_text(csi->dev, text, gstate->ctm, 0);
					fz_fill_shade(csi->dev, gstate->fill.shade, csi->top_ctm, gstate->fill.alpha);
					fz_pop_clip(csi->dev);
				}
				break;
			}
		}

		if (dostroke)
		{
			switch (gstate->stroke.kind)
			{
			case PDF_MAT_NONE:
				break;
			case PDF_MAT_COLOR:
				fz_stroke_text(csi->dev, text, gstate->stroke_state, gstate->ctm,
					gstate->stroke.colorspace, gstate->stroke.v, gstate->stroke.alpha);
				break;
			case PDF_MAT_PATTERN:
				if (gstate->stroke.pattern)
				{
					fz_clip_stroke_text(csi->dev, text, gstate->stroke_state, gstate->ctm);
					pdf_show_pattern(csi, gstate->stroke.pattern, csi->text_bbox, PDF_STROKE);
					fz_pop_clip(csi->dev);
				}
				break;
			case PDF_MAT_SHADE:
				if (gstate->stroke.shade)
				{
					fz_clip_stroke_text(csi->dev, text, gstate->stroke_state, gstate->ctm);
					fz_fill_shade(csi->dev, gstate->stroke.shade, csi->top_ctm, gstate->stroke.alpha);
					fz_pop_clip(csi->dev);
				}
				break;
			}
		}

		pdf_end_group(csi);
	}
	fz_catch(ctx)
	{
		fz_free_text(ctx, text);
		fz_rethrow(ctx);
	}

	fz_free_text(ctx, text);
}

static void
pdf_show_char(pdf_csi *csi, int cid)//��ӡ�ַ�
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_font_desc *fontdesc = gstate->font;
	fz_matrix tsm, trm;
	float w0, w1, tx, ty;
	pdf_hmtx h;
	pdf_vmtx v;
	int gid;
	int ucsbuf[8];
	int ucslen;
	int i;
	fz_rect bbox;
	int render_direct;

	tsm.a = gstate->size * gstate->scale;
	tsm.b = 0;
	tsm.c = 0;
	tsm.d = gstate->size;
	tsm.e = 0;
	tsm.f = gstate->rise;

	ucslen = 0;
	if (fontdesc->to_unicode)
		ucslen = pdf_lookup_cmap_full(fontdesc->to_unicode, cid, ucsbuf);
	if (ucslen == 0 && cid < fontdesc->cid_to_ucs_len)
	{
		ucsbuf[0] = fontdesc->cid_to_ucs[cid];
		ucslen = 1;
	}
	if (ucslen == 0 || (ucslen == 1 && ucsbuf[0] == 0))
	{
		ucsbuf[0] = '?';
		ucslen = 1;
	}

	gid = pdf_font_cid_to_gid(ctx, fontdesc, cid);

	/* cf. http://code.google.com/p/sumatrapdf/issues/detail?id=1149 */
	if (fontdesc->wmode == 1 && fontdesc->font->ft_face)
		gid = pdf_ft_lookup_vgid(ctx, fontdesc, gid);

	if (fontdesc->wmode == 1)
	{
		v = pdf_lookup_vmtx(ctx, fontdesc, cid);//vmtx��ֱ������
		tsm.e -= v.x * gstate->size * 0.001f;
		tsm.f -= v.y * gstate->size * 0.001f;
	}

	trm = fz_concat(tsm, csi->tm);//�������

	bbox = fz_bound_glyph(ctx, fontdesc->font, gid, trm);
	/* Compensate for the glyph cache limited positioning precision �������λ������޵Ķ�λ����*/
	bbox.x0 -= 1;
	bbox.y0 -= 1;
	bbox.x1 += 1;
	bbox.y1 += 1;

	render_direct = !fz_glyph_cacheable(ctx, fontdesc->font, gid);

	/* flush buffered text if face or matrix or rendermode has changed */
	if (!csi->text ||
		fontdesc->font != csi->text->font ||
		fontdesc->wmode != csi->text->wmode ||
		fabsf(trm.a - csi->text->trm.a) > FLT_EPSILON ||
		fabsf(trm.b - csi->text->trm.b) > FLT_EPSILON ||
		fabsf(trm.c - csi->text->trm.c) > FLT_EPSILON ||
		fabsf(trm.d - csi->text->trm.d) > FLT_EPSILON ||
		gstate->render != csi->text_mode ||
		render_direct)
	{
		pdf_flush_text(csi);

		csi->text = fz_new_text(ctx, fontdesc->font, trm, fontdesc->wmode);
		csi->text->trm.e = 0;
		csi->text->trm.f = 0;
		csi->text_mode = gstate->render;
		csi->text_bbox = fz_empty_rect;
	}

	if (render_direct)
	{
		/* Render the glyph stream direct here (only happens for
		 * type3 glyphs that seem to inherit current graphics
		 * attributes) ��Ⱦ������ֱ��������*/
		fz_matrix composed = fz_concat(trm, gstate->ctm);//�������
		fz_render_t3_glyph_direct(ctx, csi->dev, fontdesc->font, gid, composed, gstate);
	}
	/* SumatraPDF: still allow text extraction ��Ȼ�����ı���ȡ*/
	if (render_direct)
		csi->text_mode = 3 /* invisible ���ɼ���*/;
	{
		/* SumatraPDF: text_bbox is in device space */
		bbox = fz_transform_rect(gstate->ctm, bbox);
		csi->text_bbox = fz_union_rect(csi->text_bbox, bbox);

		/* add glyph to textobject �������*/
		fz_add_text(ctx, csi->text, gid, ucsbuf[0], trm.e, trm.f);
		
		/* add filler glyphs for one-to-many unicode mapping Ϊһ�Զ�Unicodeӳ�����������*/
		for (i = 1; i < ucslen; i++)
			fz_add_text(ctx, csi->text, -1, ucsbuf[i], trm.e, trm.f);
	}

	if (fontdesc->wmode == 0)
	{
		h = pdf_lookup_hmtx(ctx, fontdesc, cid);
		w0 = h.w * 0.001f;
		tx = (w0 * gstate->size + gstate->char_space) * gstate->scale;
		csi->tm = fz_concat(fz_translate(tx, 0), csi->tm);
	}

	if (fontdesc->wmode == 1)
	{
		w1 = v.w * 0.001f;
		ty = w1 * gstate->size + gstate->char_space;
		csi->tm = fz_concat(fz_translate(0, ty), csi->tm);
	}
}

static void
pdf_show_space(pdf_csi *csi, float tadj)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_font_desc *fontdesc = gstate->font;

	if (!fontdesc)
	{
		fz_warn(ctx, "cannot draw text since font and size not set");//δ�����������С���޷���������
		return;
	}

	if (fontdesc->wmode == 0)
		csi->tm = fz_concat(fz_translate(tadj * gstate->scale, 0), csi->tm);
	else
		csi->tm = fz_concat(fz_translate(0, tadj), csi->tm);
}

static void
pdf_show_string(pdf_csi *csi, unsigned char *buf, int len)//��ʾ�ַ���
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_font_desc *fontdesc = gstate->font;
	unsigned char *end = buf + len;
	int cpt, cid;

	if (!fontdesc)//ȷ������Ȳ����������
	{
		fz_warn(ctx, "cannot draw text since font and size not set");
		return;
	}

	while (buf < end)
	{
		int w = pdf_decode_cmap(fontdesc->encoding, buf, &cpt);//�ô���ռ䷶Χ��һ�����ֽڱ�����ַ�������ȡ�ַ������
		buf += w;

		cid = pdf_lookup_cmap(fontdesc->encoding, cpt);//����һ�������ӳ��
		if (cid >= 0)
			pdf_show_char(csi, cid);//��ӡ�ַ�
		else
			fz_warn(ctx, "cannot encode character with code point %#x", cpt);
		if (cpt == 32 && w == 1)
			pdf_show_space(csi, gstate->word_space);
	}
}

static void
pdf_show_text(pdf_csi *csi, pdf_obj *text)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	int i;

	if (pdf_is_array(text))
	{
		int n = pdf_array_len(text);
		for (i = 0; i < n; i++)
		{
			pdf_obj *item = pdf_array_get(text, i);
			if (pdf_is_string(item))
				pdf_show_string(csi, (unsigned char *)pdf_to_str_buf(item), pdf_to_str_len(item));
			else
				pdf_show_space(csi, - pdf_to_real(item) * gstate->size * 0.001f);
		}
	}
	else if (pdf_is_string(text))
	{
		pdf_show_string(csi, (unsigned char *)pdf_to_str_buf(text), pdf_to_str_len(text));
	}
}

/*
 * Interpreter and graphics state stack.
 */

static void
pdf_init_gstate(fz_context *ctx, pdf_gstate *gs, fz_matrix ctm)
{
	gs->ctm = ctm;
	gs->clip_depth = 0;

	gs->stroke_state = fz_new_stroke_state(ctx);

	gs->stroke.kind = PDF_MAT_COLOR;
	gs->stroke.colorspace = fz_device_gray; /* No fz_keep_colorspace as static */
	gs->stroke.v[0] = 0;
	gs->stroke.pattern = NULL;
	gs->stroke.shade = NULL;
	gs->stroke.alpha = 1;

	gs->fill.kind = PDF_MAT_COLOR;
	gs->fill.colorspace = fz_device_gray; /* No fz_keep_colorspace as static */
	gs->fill.v[0] = 0;
	gs->fill.pattern = NULL;
	gs->fill.shade = NULL;
	gs->fill.alpha = 1;

	gs->char_space = 0;
	gs->word_space = 0;
	gs->scale = 1;
	gs->leading = 0;
	gs->font = NULL;
	gs->size = -1;
	gs->render = 0;
	gs->rise = 0;

	gs->blendmode = 0;
	gs->softmask = NULL;
	gs->softmask_ctm = fz_identity;
	gs->luminosity = 0;
}

static pdf_material *
pdf_keep_material(fz_context *ctx, pdf_material *mat)
{
	if (mat->colorspace)
		fz_keep_colorspace(ctx, mat->colorspace);
	if (mat->pattern)
		pdf_keep_pattern(ctx, mat->pattern);
	if (mat->shade)
		fz_keep_shade(ctx, mat->shade);
	return mat;
}

static pdf_material *
pdf_drop_material(fz_context *ctx, pdf_material *mat)
{
	if (mat->colorspace)
		fz_drop_colorspace(ctx, mat->colorspace);
	if (mat->pattern)
		pdf_drop_pattern(ctx, mat->pattern);
	if (mat->shade)
		fz_drop_shade(ctx, mat->shade);
	return mat;
}

static void
copy_state(fz_context *ctx, pdf_gstate *gs, pdf_gstate *old)
{
	gs->stroke = old->stroke;
	gs->fill = old->fill;
	gs->font = old->font;
	gs->softmask = old->softmask;
	gs->stroke_state = fz_keep_stroke_state(ctx, old->stroke_state);

	pdf_keep_material(ctx, &gs->stroke);
	pdf_keep_material(ctx, &gs->fill);
	if (gs->font)
		pdf_keep_font(ctx, gs->font);
	if (gs->softmask)
		pdf_keep_xobject(ctx, gs->softmask);
}


static pdf_csi *
pdf_new_csi(pdf_document *xref, fz_device *dev, fz_matrix ctm, char *event, fz_cookie *cookie, pdf_gstate *gstate)
{
	pdf_csi *csi;
	fz_context *ctx = dev->ctx;

	csi = fz_malloc_struct(ctx, pdf_csi);
	fz_try(ctx)
	{
		csi->xref = xref;
		csi->dev = dev;
		csi->event = event;

		csi->top = 0;
		csi->obj = NULL;
		csi->name[0] = 0;
		csi->string_len = 0;
		memset(csi->stack, 0, sizeof csi->stack);

		csi->xbalance = 0;
		csi->in_text = 0;
		csi->in_hidden_ocg = 0;

		csi->path = fz_new_path(ctx);
		csi->clip = 0;
		csi->clip_even_odd = 0;

		csi->text = NULL;
		csi->tlm = fz_identity;
		csi->tm = fz_identity;
		csi->text_mode = 0;
		csi->accumulate = 1;

		csi->gcap = 64;
		csi->gstate = fz_malloc_array(ctx, csi->gcap, sizeof(pdf_gstate));

		csi->top_ctm = ctm;
		pdf_init_gstate(ctx, &csi->gstate[0], ctm);
		if (gstate)
			copy_state(ctx, &csi->gstate[0], gstate);
		csi->gtop = 0;

		csi->cookie = cookie;
	}
	fz_catch(ctx)
	{
		fz_free_path(ctx, csi->path);
		fz_free(ctx, csi);
		fz_rethrow(ctx);
	}

	return csi;
}

static void
pdf_clear_stack(pdf_csi *csi)
{
	int i;

	if (csi->obj)
		pdf_drop_obj(csi->obj);
	csi->obj = NULL;

	csi->name[0] = 0;
	csi->string_len = 0;
	for (i = 0; i < csi->top; i++)
		csi->stack[i] = 0;

	csi->top = 0;
}

static void
pdf_gsave(pdf_csi *csi)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gs;

	if (csi->gtop == csi->gcap-1)
	{
		csi->gstate = fz_resize_array(ctx, csi->gstate, csi->gcap*2, sizeof(pdf_gstate));
		csi->gcap *= 2;
	}

	memcpy(&csi->gstate[csi->gtop + 1], &csi->gstate[csi->gtop], sizeof(pdf_gstate));

	csi->gtop++;
	gs = &csi->gstate[csi->gtop];
	pdf_keep_material(ctx, &gs->stroke);
	pdf_keep_material(ctx, &gs->fill);
	if (gs->font)
		pdf_keep_font(ctx, gs->font);
	if (gs->softmask)
		pdf_keep_xobject(ctx, gs->softmask);
	fz_keep_stroke_state(ctx, gs->stroke_state);
}

static void
pdf_grestore(pdf_csi *csi)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gs = csi->gstate + csi->gtop;
	int clip_depth = gs->clip_depth;

	if (csi->gtop == 0)
	{
		fz_warn(ctx, "gstate underflow in content stream");
		return;
	}

	pdf_drop_material(ctx, &gs->stroke);
	pdf_drop_material(ctx, &gs->fill);
	if (gs->font)
		pdf_drop_font(ctx, gs->font);
	if (gs->softmask)
		pdf_drop_xobject(ctx, gs->softmask);
	fz_drop_stroke_state(ctx, gs->stroke_state);

	csi->gtop --;

	gs = csi->gstate + csi->gtop;
	while (clip_depth > gs->clip_depth)
	{
		fz_try(ctx)
		{
			fz_pop_clip(csi->dev);
		}
		fz_catch(ctx)
		{
			/* Silently swallow the problem */
		}
		clip_depth--;
	}
}

static void
pdf_free_csi(pdf_csi *csi)
{
	fz_context *ctx = csi->dev->ctx;

	while (csi->gtop)
		pdf_grestore(csi);

	pdf_drop_material(ctx, &csi->gstate[0].fill);
	pdf_drop_material(ctx, &csi->gstate[0].stroke);
	if (csi->gstate[0].font)
		pdf_drop_font(ctx, csi->gstate[0].font);
	if (csi->gstate[0].softmask)
		pdf_drop_xobject(ctx, csi->gstate[0].softmask);
	fz_drop_stroke_state(ctx, csi->gstate[0].stroke_state);

	while (csi->gstate[0].clip_depth--)
		fz_pop_clip(csi->dev);

	if (csi->path) fz_free_path(ctx, csi->path);
	if (csi->text) fz_free_text(ctx, csi->text);

	pdf_clear_stack(csi);

	fz_free(ctx, csi->gstate);

	fz_free(ctx, csi);
}

/*
 * Material state
 */

static void
pdf_set_colorspace(pdf_csi *csi, int what, fz_colorspace *colorspace)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gs = csi->gstate + csi->gtop;
	pdf_material *mat;

	pdf_flush_text(csi);

	mat = what == PDF_FILL ? &gs->fill : &gs->stroke;

	fz_drop_colorspace(ctx, mat->colorspace);

	mat->kind = PDF_MAT_COLOR;
	mat->colorspace = fz_keep_colorspace(ctx, colorspace);

	mat->v[0] = 0;
	mat->v[1] = 0;
	mat->v[2] = 0;
	mat->v[3] = 1;
}

static void
pdf_set_color(pdf_csi *csi, int what, float *v)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gs = csi->gstate + csi->gtop;
	pdf_material *mat;
	int i;

	pdf_flush_text(csi);

	mat = what == PDF_FILL ? &gs->fill : &gs->stroke;

	switch (mat->kind)
	{
	case PDF_MAT_PATTERN:
	case PDF_MAT_COLOR:
		if (!strcmp(mat->colorspace->name, "Lab"))//Labɫ�ʿռ�
		{
			mat->v[0] = v[0] / 100;
			mat->v[1] = (v[1] + 100) / 200;
			mat->v[2] = (v[2] + 100) / 200;
		}
		//if(currentstackpoint->currentroute->colorspace!=5&&currentstackpoint->currentroute->colorspace!=6)//CMYK�ռ�Ҫ��������		
		//{
			for (i = 0; i < mat->colorspace->n; i++)
			{
				mat->v[i] = v[i];
				//printf("��ɫֵ����:%d  %.2f\n",i,v[i]);
				//getline.set[getline.count].color[i]=v[i];
				//getline->set[getline->count].color[i]=v[i];//������ɫ
				currentstackpoint->currentroute->color[i]=v[i];
				if(colorchanged==1)
				{
					currentfillcolor[i]=v[i];
					//printf("�����ɫ:%d  %.2f\n",i,v[i]);
				}
				else if(colorchanged==2)
				{
					currentstrokecolor[i]=v[i];
					//printf("�����ɫ:%d  %.2f\n",i,v[i]);
				}
			}
			colorchanged=0;
		//}
		/*else
		{
			currentstackpoint->currentroute->color[0]=255*(100-mat->v[0])*(100-mat->v[3])/10000;
			currentstackpoint->currentroute->color[1]=255*(100-mat->v[1])*(100-mat->v[3])/10000;
			currentstackpoint->currentroute->color[2]=255*(100-mat->v[2])*(100-mat->v[3])/10000;
			currentcolor[0]=currentstackpoint->currentroute->color[0];
			currentcolor[1]=currentstackpoint->currentroute->color[1];
			currentcolor[2]=currentstackpoint->currentroute->color[2];
		}
		*/
		break;
	default:
		fz_warn(ctx, "color incompatible with material");
	}
}

static void
pdf_set_shade(pdf_csi *csi, int what, fz_shade *shade)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gs = csi->gstate + csi->gtop;
	pdf_material *mat;

	pdf_flush_text(csi);

	mat = what == PDF_FILL ? &gs->fill : &gs->stroke;

	if (mat->shade)
		fz_drop_shade(ctx, mat->shade);

	mat->kind = PDF_MAT_SHADE;
	mat->shade = fz_keep_shade(ctx, shade);
}

static void
pdf_set_pattern(pdf_csi *csi, int what, pdf_pattern *pat, float *v)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gs = csi->gstate + csi->gtop;
	pdf_material *mat;

	pdf_flush_text(csi);

	mat = what == PDF_FILL ? &gs->fill : &gs->stroke;

	if (mat->pattern)
		pdf_drop_pattern(ctx, mat->pattern);

	mat->kind = PDF_MAT_PATTERN;
	if (pat)
		mat->pattern = pdf_keep_pattern(ctx, pat);
	else
		mat->pattern = NULL;

	if (v)
		pdf_set_color(csi, what, v);
}

static void
pdf_unset_pattern(pdf_csi *csi, int what)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gs = csi->gstate + csi->gtop;
	pdf_material *mat;
	mat = what == PDF_FILL ? &gs->fill : &gs->stroke;
	if (mat->kind == PDF_MAT_PATTERN)
	{
		if (mat->pattern)
			pdf_drop_pattern(ctx, mat->pattern);
		mat->pattern = NULL;
		mat->kind = PDF_MAT_COLOR;
	}
}

/*
 * Patterns, XObjects and ExtGState
 */

static void
pdf_show_pattern(pdf_csi *csi, pdf_pattern *pat, fz_rect area, int what)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate;
	fz_matrix ptm, invptm;
	fz_matrix oldtopctm;
	int x0, y0, x1, y1;
	int oldtop;

	pdf_gsave(csi);
	gstate = csi->gstate + csi->gtop;

	if (pat->ismask)
	{
		pdf_unset_pattern(csi, PDF_FILL);
		pdf_unset_pattern(csi, PDF_STROKE);
		if (what == PDF_FILL)
		{
			pdf_drop_material(ctx, &gstate->stroke);
			pdf_keep_material(ctx, &gstate->fill);
			gstate->stroke = gstate->fill;
		}
		if (what == PDF_STROKE)
		{
			pdf_drop_material(ctx, &gstate->fill);
			pdf_keep_material(ctx, &gstate->stroke);
			gstate->fill = gstate->stroke;
		}
	}
	else
	{
		// TODO: unset only the current fill/stroke or both?
		pdf_unset_pattern(csi, what);
	}

	/* don't apply softmasks to objects in the pattern as well */
	if (gstate->softmask)
	{
		pdf_drop_xobject(ctx, gstate->softmask);
		gstate->softmask = NULL;
	}

	ptm = fz_concat(pat->matrix, csi->top_ctm);
	invptm = fz_invert_matrix(ptm);

	/* patterns are painted using the ctm in effect at the beginning of the content stream */
	/* get bbox of shape in pattern space for stamping */
	area = fz_transform_rect(invptm, area);

	/* When calculating the number of tiles required, we adjust by a small
	 * amount to allow for rounding errors. By choosing this amount to be
	 * smaller than 1/256, we guarantee we won't cause problems that will
	 * be visible even under our most extreme antialiasing. */
	x0 = floorf(area.x0 / pat->xstep + 0.001);
	y0 = floorf(area.y0 / pat->ystep + 0.001);
	x1 = ceilf(area.x1 / pat->xstep - 0.001);
	y1 = ceilf(area.y1 / pat->ystep - 0.001);

	oldtopctm = csi->top_ctm;
	oldtop = csi->gtop;

#ifdef TILE
	/* cf. http://code.google.com/p/sumatrapdf/issues/detail?id=1807 */
	if ((x1 - x0) * (y1 - y0) > 1 && fz_matrix_max_expansion(ptm) < 50)
#else
	if (0)
#endif
	{
		fz_begin_tile(csi->dev, area, pat->bbox, pat->xstep, pat->ystep, ptm);
		gstate->ctm = ptm;
		csi->top_ctm = gstate->ctm;
		pdf_gsave(csi);
		pdf_run_buffer(csi, pat->resources, pat->contents);
		/* RJW: "cannot render pattern tile" */
		pdf_grestore(csi);
		while (oldtop < csi->gtop)
			pdf_grestore(csi);
		fz_end_tile(csi->dev);
	}
	else
	{
		int x, y;
		for (y = y0; y < y1; y++)
		{
			for (x = x0; x < x1; x++)
			{
				gstate->ctm = fz_concat(fz_translate(x * pat->xstep, y * pat->ystep), ptm);
				csi->top_ctm = gstate->ctm;
				pdf_gsave(csi);
				fz_try(ctx)
				{
					pdf_run_buffer(csi, pat->resources, pat->contents);
				}
				fz_catch(ctx)
				{
					pdf_grestore(csi);
					while (oldtop < csi->gtop)
						pdf_grestore(csi);
					csi->top_ctm = oldtopctm;
					fz_throw(ctx, "cannot render pattern tile");
				}
				pdf_grestore(csi);
				while (oldtop < csi->gtop)
					pdf_grestore(csi);
			}
		}
	}
	csi->top_ctm = oldtopctm;

	pdf_grestore(csi);
}

static void
pdf_run_xobject(pdf_csi *csi, pdf_obj *resources, pdf_xobject *xobj, fz_matrix transform)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = NULL;
	fz_matrix oldtopctm;
	int oldtop = 0;
	int popmask;

	/* Avoid infinite recursion */
	if (xobj == NULL || pdf_dict_mark(xobj->me))
		return;

	fz_var(gstate);
	fz_var(oldtop);
	fz_var(popmask);

	fz_try(ctx)
	{
		pdf_gsave(csi);

		gstate = csi->gstate + csi->gtop;
		oldtop = csi->gtop;
		popmask = 0;

		/* apply xobject's transform matrix */
		transform = fz_concat(xobj->matrix, transform);
		gstate->ctm = fz_concat(transform, gstate->ctm);

		/* apply soft mask, create transparency group and reset state */
		if (xobj->transparency)
		{
			if (gstate->softmask)
			{
				pdf_xobject *softmask = gstate->softmask;
				fz_rect bbox = fz_transform_rect(gstate->ctm, xobj->bbox);

				gstate->softmask = NULL;
				popmask = 1;

				fz_begin_mask(csi->dev, bbox, gstate->luminosity,
					softmask->colorspace, gstate->softmask_bc);
				pdf_run_xobject(csi, resources, softmask, fz_identity);
				/* RJW: "cannot run softmask" */
				fz_end_mask(csi->dev);

				pdf_drop_xobject(ctx, softmask);
			}

			fz_begin_group(csi->dev,
				fz_transform_rect(gstate->ctm, xobj->bbox),
				xobj->isolated, xobj->knockout, gstate->blendmode, gstate->fill.alpha);

			gstate->blendmode = 0;
			gstate->stroke.alpha = 1;
			gstate->fill.alpha = 1;
		}

		/* clip to the bounds */

		fz_moveto(ctx, csi->path, xobj->bbox.x0, xobj->bbox.y0);
		fz_lineto(ctx, csi->path, xobj->bbox.x1, xobj->bbox.y0);
		fz_lineto(ctx, csi->path, xobj->bbox.x1, xobj->bbox.y1);
		fz_lineto(ctx, csi->path, xobj->bbox.x0, xobj->bbox.y1);
		fz_closepath(ctx, csi->path);
		csi->clip = 1;
		pdf_show_path(csi, 0, 0, 0, 0);

		/* run contents */

		oldtopctm = csi->top_ctm;
		csi->top_ctm = gstate->ctm;

		if (xobj->resources)
			resources = xobj->resources;

		pdf_run_buffer(csi, resources, xobj->contents);
		/* RJW: "cannot interpret XObject stream" */
	}
	fz_always(ctx)
	{
		if (gstate)
		{
			csi->top_ctm = oldtopctm;

			while (oldtop < csi->gtop)
				pdf_grestore(csi);

			pdf_grestore(csi);
		}

		pdf_dict_unmark(xobj->me);
	}
	fz_catch(ctx)
	{
		fz_rethrow(ctx);
	}

	/* wrap up transparency stacks */
	if (xobj->transparency)
	{
		fz_end_group(csi->dev);
		if (popmask)
			fz_pop_clip(csi->dev);
	}
}

static void
pdf_run_extgstate(pdf_csi *csi, pdf_obj *rdb, pdf_obj *extgstate)//����ͼ��״̬������
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	fz_colorspace *colorspace;
	int i, k, n;

	pdf_flush_text(csi);

	n = pdf_dict_len(extgstate);
	for (i = 0; i < n; i++)
	{
		pdf_obj *key = pdf_dict_get_key(extgstate, i);
		pdf_obj *val = pdf_dict_get_val(extgstate, i);
		char *s = pdf_to_name(key);

		if (!strcmp(s, "Font"))
		{
			if (pdf_is_array(val) && pdf_array_len(val) == 2)
			{
				pdf_obj *font = pdf_array_get(val, 0);

				if (gstate->font)
				{
					pdf_drop_font(ctx, gstate->font);
					gstate->font = NULL;
				}

				gstate->font = pdf_load_font(csi->xref, rdb, font);
				/* RJW: "cannot load font (%d %d R)", pdf_to_num(font), pdf_to_gen(font) */
				if (!gstate->font)
					fz_throw(ctx, "cannot find font in store");
				gstate->size = pdf_to_real(pdf_array_get(val, 1));
			}
			else
				fz_throw(ctx, "malformed /Font dictionary");
		}

		else if (!strcmp(s, "LC"))
		{
			csi->dev->flags &= ~(FZ_DEVFLAG_STARTCAP_UNDEFINED | FZ_DEVFLAG_DASHCAP_UNDEFINED | FZ_DEVFLAG_ENDCAP_UNDEFINED);
			gstate->stroke_state = fz_unshare_stroke_state(ctx, gstate->stroke_state);
			gstate->stroke_state->start_cap = pdf_to_int(val);
			gstate->stroke_state->dash_cap = pdf_to_int(val);
			gstate->stroke_state->end_cap = pdf_to_int(val);
		}
		else if (!strcmp(s, "LW"))
		{
			csi->dev->flags &= ~FZ_DEVFLAG_LINEWIDTH_UNDEFINED;
			gstate->stroke_state = fz_unshare_stroke_state(ctx, gstate->stroke_state);
			gstate->stroke_state->linewidth = pdf_to_real(val);
		}
		else if (!strcmp(s, "LJ"))
		{
			csi->dev->flags &= ~FZ_DEVFLAG_LINEJOIN_UNDEFINED;
			gstate->stroke_state = fz_unshare_stroke_state(ctx, gstate->stroke_state);
			gstate->stroke_state->linejoin = pdf_to_int(val);
		}
		else if (!strcmp(s, "ML"))
		{
			csi->dev->flags &= ~FZ_DEVFLAG_MITERLIMIT_UNDEFINED;
			gstate->stroke_state = fz_unshare_stroke_state(ctx, gstate->stroke_state);
			gstate->stroke_state->miterlimit = pdf_to_real(val);
		}

		else if (!strcmp(s, "D"))
		{
			if (pdf_is_array(val) && pdf_array_len(val) == 2)
			{
				pdf_obj *dashes = pdf_array_get(val, 0);
				int len = pdf_array_len(dashes);
				gstate->stroke_state = fz_unshare_stroke_state_with_len(ctx, gstate->stroke_state, len);
				gstate->stroke_state->dash_len = len;
				for (k = 0; k < len; k++)
					gstate->stroke_state->dash_list[k] = pdf_to_real(pdf_array_get(dashes, k));
				gstate->stroke_state->dash_phase = pdf_to_real(pdf_array_get(val, 1));
			}
			else
				fz_throw(ctx, "malformed /D");
		}

		else if (!strcmp(s, "CA"))
		{
			gstate->stroke.alpha = pdf_to_real(val);//�˴�Ϊ͸����
			currentstackpoint->ca=gstate->stroke.alpha;
			//printf("��ȡ͸����%f",gstate->stroke.alpha);
		}
		else if (!strcmp(s, "ca"))
			gstate->fill.alpha = pdf_to_real(val);

		else if (!strcmp(s, "BM"))
		{
			if (pdf_is_array(val))
				val = pdf_array_get(val, 0);
			gstate->blendmode = fz_lookup_blendmode(pdf_to_name(val));
		}

		else if (!strcmp(s, "SMask"))
		{
			if (pdf_is_dict(val))
			{
				pdf_xobject *xobj;
				pdf_obj *group, *luminosity, *bc;

				if (gstate->softmask)
				{
					pdf_drop_xobject(ctx, gstate->softmask);
					gstate->softmask = NULL;
				}

				group = pdf_dict_gets(val, "G");
				if (!group)
					fz_throw(ctx, "cannot load softmask xobject (%d %d R)", pdf_to_num(val), pdf_to_gen(val));
				xobj = pdf_load_xobject(csi->xref, group);
				/* RJW: "cannot load xobject (%d %d R)", pdf_to_num(val), pdf_to_gen(val) */

				colorspace = xobj->colorspace;
				if (!colorspace)
					colorspace = fz_device_gray;

				gstate->softmask_ctm = fz_concat(xobj->matrix, gstate->ctm);
				gstate->softmask = xobj;
				for (k = 0; k < colorspace->n; k++)
					gstate->softmask_bc[k] = 0;

				bc = pdf_dict_gets(val, "BC");
				if (pdf_is_array(bc))
				{
					for (k = 0; k < colorspace->n; k++)
						gstate->softmask_bc[k] = pdf_to_real(pdf_array_get(bc, k));
				}

				luminosity = pdf_dict_gets(val, "S");
				if (pdf_is_name(luminosity) && !strcmp(pdf_to_name(luminosity), "Luminosity"))
					gstate->luminosity = 1;
				else
					gstate->luminosity = 0;
			}
			else if (pdf_is_name(val) && !strcmp(pdf_to_name(val), "None"))
			{
				if (gstate->softmask)
				{
					pdf_drop_xobject(ctx, gstate->softmask);
					gstate->softmask = NULL;
				}
			}
		}

		else if (!strcmp(s, "TR"))
		{
			if (!pdf_is_name(val) || strcmp(pdf_to_name(val), "Identity"))
				fz_warn(ctx, "ignoring transfer function");
		}
	}
}

/*
 * Operators
 */

static void pdf_run_BDC(pdf_csi *csi, pdf_obj *rdb)
{
	pdf_obj *ocg;
	stackstate++;
	
	if(stackstate==1)
	{
		currentstackpoint->existstack=1;
		//printf("����stack:����� %d\n",currentstackpoint->existnest);
		//addneststack(currentstackpoint);
		push(currentstackpoint);
	}
	else
	{
		currentstackpoint->existnest=1;
		//printf("����stack: %d\n",currentstackpoint->existnest);
		addneststack(currentstackpoint);//��ǰstack���Ƕ��
		currentstackpoint=currentstackpoint->currentstack;//��ǰջָ�������ƶ�һ��
		push(currentstackpoint);
		currentstackpoint->existstack=1;
	}
	/* If we are already in a hidden OCG, then we'll still be hidden -
	 * just increment the depth so we pop back to visibility when we've
	 * seen enough EDCs. */
	if (csi->in_hidden_ocg > 0)
	{
		csi->in_hidden_ocg++;
		return;
	}

	ocg = pdf_dict_gets(pdf_dict_gets(rdb, "Properties"), csi->name);//ȥ�ֵ���Ѱ��
	if (!ocg)
	{
		/* No Properties array, or name not found in the properties
		 * means visible. */
		return;
	}
	if (strcmp(pdf_to_name(pdf_dict_gets(ocg, "Type")), "OCG") != 0)
	{
		/* Wrong type of property */
		return;
	}
	if (pdf_is_hidden_ocg(ocg, csi, rdb))
		csi->in_hidden_ocg++;
}

static void pdf_run_BI(pdf_csi *csi, pdf_obj *rdb, fz_stream *file)
{
	fz_context *ctx = csi->dev->ctx;
	int ch;
	fz_image *img;
	pdf_obj *obj;

	obj = pdf_parse_dict(csi->xref, file, &csi->xref->lexbuf.base);
	/* RJW: "cannot parse inline image dictionary" */

	/* read whitespace after ID keyword */
	ch = fz_read_byte(file);
	if (ch == '\r')
		if (fz_peek_byte(file) == '\n')
			fz_read_byte(file);

	img = pdf_load_inline_image(csi->xref, rdb, obj, file);
	pdf_drop_obj(obj);
	/* RJW: "cannot load inline image" */

	pdf_show_image(csi, img);

	fz_drop_image(ctx, img);

	/* find EI */
	ch = fz_read_byte(file);
	while (ch != 'E' && ch != EOF)
		ch = fz_read_byte(file);
	ch = fz_read_byte(file);
	if (ch != 'I')
		fz_throw(ctx, "syntax error after inline image");
}

static void pdf_run_B(pdf_csi *csi)//B���֮��ͿĨ ������������
{
	pdf_show_path(csi, 0, 1, 1, 0);
	currentstackpoint->currentroute->drawingmethord=5;
	currentstackpoint->countroute++;
	if(currentstackpoint->currentroute->colorspace==-1)//��ɫ�ʿռ䲻��,����˳��
	{
		//getline->set[getline->count].colorspace=currentcolorspace;
		currentstackpoint->currentroute->colorspace=currentfillcolorspace;
		currentstackpoint->currentroute->scolorspace=currentstrokecolorspace;
		printf("��ǰ˫ɫ����ɫ�ռ� %d %d\n",currentstackpoint->currentroute->colorspace,currentstackpoint->currentroute->scolorspace);
		//printf("����:currentcolorspace:  %d\n",currentcolorspace);
		//printf("����:˳��RGBɫ�ʿռ� %.3f %.3f %.3f\n",currentcolor[0],currentcolor[1],currentcolor[2]);
		if(currentfillcolorspace==3||currentfillcolorspace==4)
		{//�����ҲͿĨ,Ҫ����������ɫ
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			getline->set[getline->count].color[1]=currentcolor[1];
			getline->set[getline->count].color[2]=currentcolor[2];
			*/
			currentstackpoint->currentroute->color[0]=currentfillcolor[0];
			currentstackpoint->currentroute->color[1]=currentfillcolor[1];
			currentstackpoint->currentroute->color[2]=currentfillcolor[2];
		}
		else
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			*/
			currentstackpoint->currentroute->color[0]=currentfillcolor[0];
		}
		currentstackpoint->currentroute->scolor=(float*)malloc(4*sizeof(float));
		//Ϊscolor����ռ�
		if(currentstrokecolorspace==3||currentstrokecolorspace==4)
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			getline->set[getline->count].color[1]=currentcolor[1];
			getline->set[getline->count].color[2]=currentcolor[2];
			*/
			currentstackpoint->currentroute->scolor[0]=currentstrokecolor[0];
			currentstackpoint->currentroute->scolor[1]=currentstrokecolor[1];
			currentstackpoint->currentroute->scolor[2]=currentstrokecolor[2];
		}
		else
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			*/
			currentstackpoint->currentroute->scolor[0]=currentfillcolor[0];
		}
		
	}
	//printf("\n��ǰ�ṹ�Ƿ�Ϊջ:%d\n",currentstackpoint->existstack);
	if(currentstackpoint->existstack!=0)
	{
		addroute(currentstackpoint);//������ջ�������·��  (��֮�����������һ������ĵ�����)
	}
	else//����Ϊ�����
	{
		currentstackpoint->countroute=1;//�����ֻ��һ��·��
		addstack(getline);
		currentstackpoint=getline->currentstack;
		getline->count++;
	}
}

static void pdf_run_BMC(pdf_csi *csi)
{
	/* If we are already in a hidden OCG, then we'll still be hidden -
	 * just increment the depth so we pop back to visibility when we've
	 * seen enough EDCs. */
	stackstate++;
	currentstackpoint->existstack=1;
	if(stackstate==1)
	{
		//printf("����stack:����� %d\n",currentstackpoint->existnest);
		//addneststack(currentstackpoint);
		push(currentstackpoint);
	}
	else
	{
		currentstackpoint->existnest=1;
		//printf("����stack: %d\n",currentstackpoint->existnest);
		addneststack(currentstackpoint);
		currentstackpoint=currentstackpoint->currentstack;//��ǰջָ�������ƶ�һ��
		push(currentstackpoint);
	}
	if (csi->in_hidden_ocg > 0)
	{
		csi->in_hidden_ocg++;
	}
}

static void pdf_run_BT(pdf_csi *csi)//��ʼһ���ı�����
{
	csi->in_text = 1;
	csi->tm = fz_identity;//��ʼ�� ��λ����
	csi->tlm = fz_identity;//ͬ��Ϊ��λ����
}

static void pdf_run_BX(pdf_csi *csi)
{
	csi->xbalance ++;
}

static void pdf_run_Bstar(pdf_csi *csi)//B* ���ͿĨ ��ż����
{
	pdf_show_path(csi, 0, 1, 1, 1);
	currentstackpoint->currentroute->drawingmethord=6;
	currentstackpoint->countroute++;
	if(currentstackpoint->currentroute->colorspace==-1)//��ɫ�ʿռ䲻��,����˳��
	{
		//getline->set[getline->count].colorspace=currentcolorspace;
		currentstackpoint->currentroute->colorspace=currentfillcolorspace;
		currentstackpoint->currentroute->scolorspace=currentstrokecolorspace;
		//printf("����:currentcolorspace:  %d\n",currentcolorspace);
		//printf("����:˳��RGBɫ�ʿռ� %.3f %.3f %.3f\n",currentcolor[0],currentcolor[1],currentcolor[2]);
		if(currentfillcolorspace==3||currentfillcolorspace==4)
		{//�����ҲͿĨ,Ҫ����������ɫ
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			getline->set[getline->count].color[1]=currentcolor[1];
			getline->set[getline->count].color[2]=currentcolor[2];
			*/
			currentstackpoint->currentroute->color[0]=currentfillcolor[0];
			currentstackpoint->currentroute->color[1]=currentfillcolor[1];
			currentstackpoint->currentroute->color[2]=currentfillcolor[2];
		}
		else
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			*/
			currentstackpoint->currentroute->color[0]=currentfillcolor[0];
		}
		currentstackpoint->currentroute->scolor=(float*)malloc(4*sizeof(float));
		//Ϊscolor����ռ�
		if(currentstrokecolorspace==3||currentstrokecolorspace==4)
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			getline->set[getline->count].color[1]=currentcolor[1];
			getline->set[getline->count].color[2]=currentcolor[2];
			*/
			currentstackpoint->currentroute->scolor[0]=currentstrokecolor[0];
			currentstackpoint->currentroute->scolor[1]=currentstrokecolor[1];
			currentstackpoint->currentroute->scolor[2]=currentstrokecolor[2];
		}
		else
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			*/
			currentstackpoint->currentroute->scolor[0]=currentfillcolor[0];
		}
		
	}
	//printf("\n��ǰ�ṹ�Ƿ�Ϊջ:%d\n",currentstackpoint->existstack);
	if(currentstackpoint->existstack!=0)
	{
		addroute(currentstackpoint);//������ջ�������·��  (��֮�����������һ������ĵ�����)
	}
	else//����Ϊ�����
	{
		currentstackpoint->countroute=1;//�����ֻ��һ��·��
		addstack(getline);
		currentstackpoint=getline->currentstack;
		getline->count++;
	}
}

static void pdf_run_cs_imp(pdf_csi *csi, pdf_obj *rdb, int what)
{
	fz_context *ctx = csi->dev->ctx;
	fz_colorspace *colorspace;
	pdf_obj *obj, *dict;

	if (!strcmp(csi->name, "Pattern"))
	{
		pdf_set_pattern(csi, what, NULL, NULL);
	}
	else
	{
		if (!strcmp(csi->name, "DeviceGray"))
			colorspace = fz_device_gray; /* No fz_keep_colorspace as static */
		else if (!strcmp(csi->name, "DeviceRGB"))
			colorspace = fz_device_rgb; /* No fz_keep_colorspace as static */
		else if (!strcmp(csi->name, "DeviceCMYK"))
			colorspace = fz_device_cmyk; /* No fz_keep_colorspace as static */
		else
		{
			dict = pdf_dict_gets(rdb, "ColorSpace");
			if (!dict)
				fz_throw(ctx, "cannot find ColorSpace dictionary");
			obj = pdf_dict_gets(dict, csi->name);
			if (!obj)
				fz_throw(ctx, "cannot find colorspace resource '%s'", csi->name);
			colorspace = pdf_load_colorspace(csi->xref, obj);
			/* RJW: "cannot load colorspace (%d 0 R)", pdf_to_num(obj) */
		}

		pdf_set_colorspace(csi, what, colorspace);

		fz_drop_colorspace(ctx, colorspace);
	}
}

static void pdf_run_CS(pdf_csi *csi, pdf_obj *rdb)
{
	csi->dev->flags &= ~FZ_DEVFLAG_STROKECOLOR_UNDEFINED;

	pdf_run_cs_imp(csi, rdb, PDF_STROKE);
	/* RJW: "cannot set colorspace" */
}

static void pdf_run_cs(pdf_csi *csi, pdf_obj *rdb)
{
	csi->dev->flags &= ~FZ_DEVFLAG_FILLCOLOR_UNDEFINED;

	pdf_run_cs_imp(csi, rdb, PDF_FILL);
	/* RJW: "cannot set colorspace" */
}

static void pdf_run_DP(pdf_csi *csi)
{
}

static void pdf_run_Do(pdf_csi *csi, pdf_obj *rdb)//����XObject
{
	fz_context *ctx = csi->dev->ctx;
	pdf_obj *dict;
	pdf_obj *obj;
	pdf_obj *subtype;

	dict = pdf_dict_gets(rdb, "XObject");
	if (!dict)
		fz_throw(ctx, "cannot find XObject dictionary when looking for: '%s'", csi->name);

	obj = pdf_dict_gets(dict, csi->name);
	if (!obj)
		fz_throw(ctx, "cannot find xobject resource: '%s'", csi->name);

	subtype = pdf_dict_gets(obj, "Subtype");
	if (!pdf_is_name(subtype))
		fz_throw(ctx, "no XObject subtype specified ͼ������δ��ȷ�涨");

	if (pdf_is_hidden_ocg(pdf_dict_gets(obj, "OC"), csi, rdb))
		return;

	if (!strcmp(pdf_to_name(subtype), "Form") && pdf_dict_gets(obj, "Subtype2"))
		subtype = pdf_dict_gets(obj, "Subtype2");

	if (!strcmp(pdf_to_name(subtype), "Form"))
	{
		pdf_xobject *xobj;

		xobj = pdf_load_xobject(csi->xref, obj);
		/* RJW: "cannot load xobject (%d %d R)", pdf_to_num(obj), pdf_to_gen(obj) */

		/* Inherit parent resources, in case this one was empty XXX check where it's loaded �̳и������Դ�Է���һ����Ϊ��*/
		if (!xobj->resources)
			xobj->resources = pdf_keep_obj(rdb);

		fz_try(ctx)
		{
			pdf_run_xobject(csi, xobj->resources, xobj, fz_identity);
		}
		fz_catch(ctx)
		{
			pdf_drop_xobject(ctx, xobj);
			fz_throw(ctx, "cannot draw xobject (%d %d R)", pdf_to_num(obj), pdf_to_gen(obj));
		}

		pdf_drop_xobject(ctx, xobj);
	}

	else if (!strcmp(pdf_to_name(subtype), "Image"))//ͼƬ
	{
		if ((csi->dev->hints & FZ_IGNORE_IMAGE) == 0)
		{
			fz_image *img = pdf_load_image(csi->xref, obj);
			/* RJW: "cannot load image (%d %d R)", pdf_to_num(obj), pdf_to_gen(obj) */
			fz_try(ctx)
			{
				pdf_show_image(csi, img);
			}
			fz_always(ctx)
			{
				fz_drop_image(ctx, img);
			}
			fz_catch(ctx)
			{
				fz_rethrow(ctx);
			}
		}
	}

	else if (!strcmp(pdf_to_name(subtype), "PS"))
	{
		fz_warn(ctx, "ignoring XObject with subtype PS");
	}

	else
	{
		fz_throw(ctx, "unknown XObject subtype: '%s'", pdf_to_name(subtype));
	}
}

static void pdf_run_EMC(pdf_csi *csi)
{
	stackstate--;
	if(stackstate==0)
	{
		//printf("����emc��Ӵ���\n");
		pop();
		addstack(getline);	
		currentstackpoint=getline->currentstack;
		getline->count++;
	}
	else
	{
		pop();
		currentstackpoint=currentstack();//������һ��
	}
	/*if(stackstate==0)//����ջ�ڵ�ʱ��
	{
		getline->count=getline->count+1;//·�����
		addstack(getline);
	}*/
	if (csi->in_hidden_ocg > 0)
		csi->in_hidden_ocg--;
}

static void pdf_run_ET(pdf_csi *csi)
{
	pdf_flush_text(csi);
	csi->accumulate = 1;
	csi->in_text = 0;
}

static void pdf_run_EX(pdf_csi *csi)
{
	csi->xbalance --;
}

static void pdf_run_F(pdf_csi *csi)
{
	pdf_show_path(csi, 0, 1, 0, 0);
	currentstackpoint->currentroute->drawingmethord=3;
	currentstackpoint->countroute++;
	if(currentstackpoint->currentroute->colorspace==-1)//��ɫ�ʿռ䲻��,����˳��
	{
		//getline->set[getline->count].colorspace=currentcolorspace;
		currentstackpoint->currentroute->colorspace=currentfillcolorspace;
		//printf("����:currentcolorspace:  %d\n",currentcolorspace);
		//printf("����:˳��RGBɫ�ʿռ� %.3f %.3f %.3f\n",currentcolor[0],currentcolor[1],currentcolor[2]);
		if(currentfillcolorspace==3||currentfillcolorspace==4)
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			getline->set[getline->count].color[1]=currentcolor[1];
			getline->set[getline->count].color[2]=currentcolor[2];
			*/
			currentstackpoint->currentroute->color[0]=currentfillcolor[0];
			currentstackpoint->currentroute->color[1]=currentfillcolor[1];
			currentstackpoint->currentroute->color[2]=currentfillcolor[2];
		}
		else
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			*/
			currentstackpoint->currentroute->color[0]=currentfillcolor[0];
		}
		
	}
	//printf("\n��ǰ�ṹ�Ƿ�Ϊջ:%d\n",currentstackpoint->existstack);
	if(currentstackpoint->existstack!=0)
	{
		addroute(currentstackpoint);//������ջ�������·��  (��֮�����������һ������ĵ�����)
	}
	else//����Ϊ�����
	{
		currentstackpoint->countroute=1;//�����ֻ��һ��·��
		addstack(getline);
		currentstackpoint=getline->currentstack;
		getline->count++;
	}
}

static void pdf_run_G(pdf_csi *csi)
{
	colorchanged=2;
	csi->dev->flags &= ~FZ_DEVFLAG_STROKECOLOR_UNDEFINED;
	pdf_set_colorspace(csi, PDF_STROKE, fz_device_gray);
	pdf_set_color(csi, PDF_STROKE, csi->stack);
	//getline->set[getline->count].colorspace=1;//������ɫ�ռ�Ϊ1����G
	//currentstackpoint->currentroute->colorspace=1;
	currentstrokecolorspace=1;//���浱ǰ��ɫ�ռ�,�Ա�˳��ʹ��
}

static void pdf_run_J(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	csi->dev->flags &= ~(FZ_DEVFLAG_STARTCAP_UNDEFINED | FZ_DEVFLAG_DASHCAP_UNDEFINED | FZ_DEVFLAG_ENDCAP_UNDEFINED);
	gstate->stroke_state = fz_unshare_stroke_state(csi->dev->ctx, gstate->stroke_state);
	gstate->stroke_state->start_cap = csi->stack[0];
	gstate->stroke_state->dash_cap = csi->stack[0];
	gstate->stroke_state->end_cap = csi->stack[0];
#ifdef debug
	printf("�߶˿���ʽ:%f\n",csi->stack[0]);
#endif
}

static void pdf_run_K(pdf_csi *csi)
{
	colorchanged=2;
	csi->dev->flags &= ~FZ_DEVFLAG_STROKECOLOR_UNDEFINED;
	pdf_set_colorspace(csi, PDF_STROKE, fz_device_cmyk);
	currentstackpoint->currentroute->colorspace=6;//cmykɫ�ʿռ� K
	currentstrokecolorspace=6;//ת��Ϊrgb�ռ�
	pdf_set_color(csi, PDF_STROKE, csi->stack);
}

static void pdf_run_M(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	csi->dev->flags &= ~FZ_DEVFLAG_MITERLIMIT_UNDEFINED;
	gstate->stroke_state = fz_unshare_stroke_state(csi->dev->ctx, gstate->stroke_state);
	gstate->stroke_state->miterlimit = csi->stack[0];
}

static void pdf_run_MP(pdf_csi *csi)
{
}

static void pdf_run_Q(pdf_csi *csi)
{
	stackstate--;//��ջ
	if(stackstate==0)
	{
		pop();
		addstack(getline);	
		currentstackpoint=getline->currentstack;
		getline->count++;
	}
	else
	{
		pop();
		currentstackpoint=currentstack();//������һ��
	}
	/*if(stackstate==0)//����ջ�ڵ�ʱ��
	{
		getline->count=getline->count+1;//·�����
		addstack(getline);
	}
	//addstack(getline);
	*/
	pdf_grestore(csi);
}

static void pdf_run_RG(pdf_csi *csi)
{
	colorchanged=2;
	csi->dev->flags &= ~FZ_DEVFLAG_STROKECOLOR_UNDEFINED;
	pdf_set_colorspace(csi, PDF_STROKE, fz_device_rgb);
	pdf_set_color(csi, PDF_STROKE, csi->stack);
	//getline.set[getline.count].colorspace=3;//����RGB
	//currentstackpoint->currentroute->colorspace=3;
	currentstrokecolorspace=3;//���õ�ǰ��ɫ�ռ� �Ա�˳��
}

static void pdf_run_S(pdf_csi *csi)
{
	/*if(getline.set[getline.count].points[getline.set[getline.count].countpoint-1][2]!=3)//Ӧ�� m,l,l,l,S �����
	{
		getline.count++;
	}*/
	//getline.count++;//һ��·�����
	//getline->set[getline->count].drawingmethord=1;//���û��Ʒ��� 1-S
	currentstackpoint->currentroute->drawingmethord=1;
	currentstackpoint->countroute++;
	if(currentstackpoint->currentroute->colorspace==-1)
	{
		currentstackpoint->currentroute->colorspace=currentstrokecolorspace;
		//getline->set[getline->count].colorspace=currentcolorspace;
		//printf("����:currentcolorspace:  %d\n",currentcolorspace);
		//rintf("����:˳��RGBɫ�ʿռ� %.3f %.3f %.3f\n",currentcolor[0],currentcolor[1],currentcolor[2]);
		if(currentstrokecolorspace==3||currentstrokecolorspace==4)
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			getline->set[getline->count].color[1]=currentcolor[1];
			getline->set[getline->count].color[2]=currentcolor[2];
			*/
			currentstackpoint->currentroute->color[0]=currentstrokecolor[0];
			currentstackpoint->currentroute->color[1]=currentstrokecolor[1];
			currentstackpoint->currentroute->color[2]=currentstrokecolor[2];
		}
		else
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			*/
			currentstackpoint->currentroute->color[0]=currentstrokecolor[0];
		}	
	}
	if(currentstackpoint->currentroute->linewidth==-1)//�߿�˳��
	{
		currentstackpoint->currentroute->linewidth=currentlinewidth;
	}
	if(currentstackpoint->existstack!=0)
	{
		addroute(currentstackpoint);//������ջ�������·��  (��֮�����������һ������ĵ�����)
	}
	else//����Ϊ�����
	{
		currentstackpoint->countroute=1;//�����ֻ��һ��·��
		addstack(getline);
		currentstackpoint=getline->currentstack;
		getline->count++;
	}

	/*if(stackstate==0)//����ջ�ڵ�ʱ��
	{
		getline->count=getline->count+1;//·�����
		addstack(getline);
	}
	else//��Ȼ��ջ�� ���½�·��
	{
		addroute(getline->currentstack);
		getline->currentstack->countroute++;
	}
	*/
	
	pdf_show_path(csi, 0, 0, 1, 0);
}

static void pdf_run_SC_imp(pdf_csi *csi, pdf_obj *rdb, int what, pdf_material *mat)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_obj *patterntype;
	pdf_obj *dict;
	pdf_obj *obj;
	int kind;

	kind = mat->kind;
	if (csi->name[0])
		kind = PDF_MAT_PATTERN;

	switch (kind)
	{
	case PDF_MAT_NONE:
		fz_throw(ctx, "cannot set color in mask objects");

	case PDF_MAT_COLOR:
		pdf_set_color(csi, what, csi->stack);
		break;

	case PDF_MAT_PATTERN:
		dict = pdf_dict_gets(rdb, "Pattern");
		if (!dict)
			fz_throw(ctx, "cannot find Pattern dictionary");

		obj = pdf_dict_gets(dict, csi->name);
		if (!obj)
			fz_throw(ctx, "cannot find pattern resource '%s'", csi->name);

		patterntype = pdf_dict_gets(obj, "PatternType");

		if (pdf_to_int(patterntype) == 1)
		{
			pdf_pattern *pat;
			pat = pdf_load_pattern(csi->xref, obj);
			/* RJW: "cannot load pattern (%d 0 R)", pdf_to_num(obj) */
			pdf_set_pattern(csi, what, pat, csi->top > 0 ? csi->stack : NULL);
			pdf_drop_pattern(ctx, pat);
		}
		else if (pdf_to_int(patterntype) == 2)
		{
			fz_shade *shd;
			shd = pdf_load_shading(csi->xref, obj);
			/* RJW: "cannot load shading (%d 0 R)", pdf_to_num(obj) */
			pdf_set_shade(csi, what, shd);
			fz_drop_shade(ctx, shd);
		}
		else
		{
			fz_throw(ctx, "unknown pattern type: %d", pdf_to_int(patterntype));
		}
		break;

	case PDF_MAT_SHADE:
		fz_throw(ctx, "cannot set color in shade objects");
	}
}

static void pdf_run_SC(pdf_csi *csi, pdf_obj *rdb)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	csi->dev->flags &= ~FZ_DEVFLAG_STROKECOLOR_UNDEFINED;
	colorchanged=2;
	pdf_run_SC_imp(csi, rdb, PDF_STROKE, &gstate->stroke);
	
	//currentstackpoint->currentroute->colorspace=3;
	currentstrokecolorspace=3;//���õ�ǰ��ɫ�ռ� �Ա�˳��
	/* RJW: "cannot set color and colorspace" */
}

static void pdf_run_sc(pdf_csi *csi, pdf_obj *rdb)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	csi->dev->flags &= ~FZ_DEVFLAG_FILLCOLOR_UNDEFINED;
	colorchanged=1;
	pdf_run_SC_imp(csi, rdb, PDF_FILL, &gstate->fill);

	//currentstackpoint->currentroute->colorspace=4;
	currentfillcolorspace=4;//���õ�ǰ��ɫ�ռ� �Ա�˳��
	/* RJW: "cannot set color and colorspace" */
}

static void pdf_run_Tc(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	gstate->char_space = csi->stack[0];
}

static void pdf_run_Tw(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	gstate->word_space = csi->stack[0];
}

static void pdf_run_Tz(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	float a = csi->stack[0] / 100;
	pdf_flush_text(csi);
	gstate->scale = a;
}

static void pdf_run_TL(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	gstate->leading = csi->stack[0];
}

static void pdf_run_Tf(pdf_csi *csi, pdf_obj *rdb)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_obj *dict;
	pdf_obj *obj;

	gstate->size = csi->stack[0];
	if (gstate->font)
		pdf_drop_font(ctx, gstate->font);
	gstate->font = NULL;

	dict = pdf_dict_gets(rdb, "Font");
	if (!dict)
		fz_throw(ctx, "cannot find Font dictionary");

	obj = pdf_dict_gets(dict, csi->name);
	if (!obj)
		fz_throw(ctx, "cannot find font resource: '%s'", csi->name);

	gstate->font = pdf_load_font(csi->xref, rdb, obj);
	/* RJW: "cannot load font (%d 0 R)", pdf_to_num(obj) */
}

static void pdf_run_Tr(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	gstate->render = csi->stack[0];
}

static void pdf_run_Ts(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	gstate->rise = csi->stack[0];
}

static void pdf_run_Td(pdf_csi *csi)
{
	fz_matrix m = fz_translate(csi->stack[0], csi->stack[1]);
	csi->tlm = fz_concat(m, csi->tlm);
	csi->tm = csi->tlm;
}

static void pdf_run_TD(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	fz_matrix m;

	gstate->leading = -csi->stack[1];
	m = fz_translate(csi->stack[0], csi->stack[1]);
	csi->tlm = fz_concat(m, csi->tlm);
	csi->tm = csi->tlm;
}

static void pdf_run_Tm(pdf_csi *csi)
{
	csi->tm.a = csi->stack[0];
	csi->tm.b = csi->stack[1];
	csi->tm.c = csi->stack[2];
	csi->tm.d = csi->stack[3];
	csi->tm.e = csi->stack[4];
	csi->tm.f = csi->stack[5];
	csi->tlm = csi->tm;
}

static void pdf_run_Tstar(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	fz_matrix m = fz_translate(0, -gstate->leading);
	csi->tlm = fz_concat(m, csi->tlm);
	csi->tm = csi->tlm;
}

static void pdf_run_Tj(pdf_csi *csi)
{
	if (csi->string_len)
		pdf_show_string(csi, csi->string, csi->string_len);
	else
		pdf_show_text(csi, csi->obj);
}

static void pdf_run_TJ(pdf_csi *csi)
{
	if (csi->string_len)
		pdf_show_string(csi, csi->string, csi->string_len);
	else
		pdf_show_text(csi, csi->obj);
}

static void pdf_run_W(pdf_csi *csi)
{
	//getline->set[getline->count].type=4;//Ϊ����·��
	//getline->current->type=4;
	//��getline->current->points�����ݸ��Ƹ�getline->current->clipregion
	currentstackpoint->existclip=1;
	//�˴����Ʋü�·��
	copycliproute(currentstackpoint);
	csi->clip = 1;
	csi->clip_even_odd = 0;
}

static void pdf_run_Wstar(pdf_csi *csi)
{
	//getline->set[getline->count].type=4;//Ϊ����·��
	//getline->current->type=4;
	currentstackpoint->existclip=2;//W*����  �Դ���Ϊ����
	//�˴����Ʋü�·��
	copycliproute(currentstackpoint);
	csi->clip = 1;
	csi->clip_even_odd = 1;
}

static void pdf_run_b(pdf_csi *csi)//�ر�Ȼ����䲢ͿĨ·�� �൱��h+B
{
	pdf_show_path(csi, 1, 1, 1, 0);
	currentstackpoint->currentroute->drawingmethord=7;
	currentstackpoint->countroute++;
	if(currentstackpoint->currentroute->colorspace==-1)//��ɫ�ʿռ䲻��,����˳��
	{
		//getline->set[getline->count].colorspace=currentcolorspace;
		currentstackpoint->currentroute->colorspace=currentfillcolorspace;
		currentstackpoint->currentroute->scolorspace=currentstrokecolorspace;
		//printf("����:currentcolorspace:  %d\n",currentcolorspace);
		//printf("����:˳��RGBɫ�ʿռ� %.3f %.3f %.3f\n",currentcolor[0],currentcolor[1],currentcolor[2]);
		if(currentfillcolorspace==3||currentfillcolorspace==4)
		{//�����ҲͿĨ,Ҫ����������ɫ
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			getline->set[getline->count].color[1]=currentcolor[1];
			getline->set[getline->count].color[2]=currentcolor[2];
			*/
			currentstackpoint->currentroute->color[0]=currentfillcolor[0];
			currentstackpoint->currentroute->color[1]=currentfillcolor[1];
			currentstackpoint->currentroute->color[2]=currentfillcolor[2];
		}
		else
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			*/
			currentstackpoint->currentroute->color[0]=currentfillcolor[0];
		}
		currentstackpoint->currentroute->scolor=(float*)malloc(4*sizeof(float));
		//Ϊscolor����ռ�
		if(currentstrokecolorspace==3||currentstrokecolorspace==4)
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			getline->set[getline->count].color[1]=currentcolor[1];
			getline->set[getline->count].color[2]=currentcolor[2];
			*/
			currentstackpoint->currentroute->scolor[0]=currentstrokecolor[0];
			currentstackpoint->currentroute->scolor[1]=currentstrokecolor[1];
			currentstackpoint->currentroute->scolor[2]=currentstrokecolor[2];
		}
		else
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			*/
			currentstackpoint->currentroute->scolor[0]=currentfillcolor[0];
		}
		
	}
	//printf("\n��ǰ�ṹ�Ƿ�Ϊջ:%d\n",currentstackpoint->existstack);
	if(currentstackpoint->existstack!=0)
	{
		addroute(currentstackpoint);//������ջ�������·��  (��֮�����������һ������ĵ�����)
	}
	else//����Ϊ�����
	{
		currentstackpoint->countroute=1;//�����ֻ��һ��·��
		addstack(getline);
		currentstackpoint=getline->currentstack;
		getline->count++;
	}
}

static void pdf_run_bstar(pdf_csi *csi)//�ر����ͿĨ·�� ��żԭ�����
{
	pdf_show_path(csi, 1, 1, 1, 1);
	currentstackpoint->currentroute->drawingmethord=8;
	currentstackpoint->countroute++;
	if(currentstackpoint->currentroute->colorspace==-1)//��ɫ�ʿռ䲻��,����˳��
	{
		//getline->set[getline->count].colorspace=currentcolorspace;
		currentstackpoint->currentroute->colorspace=currentfillcolorspace;
		currentstackpoint->currentroute->scolorspace=currentstrokecolorspace;
		//printf("����:currentcolorspace:  %d\n",currentcolorspace);
		//printf("����:˳��RGBɫ�ʿռ� %.3f %.3f %.3f\n",currentcolor[0],currentcolor[1],currentcolor[2]);
		if(currentfillcolorspace==3||currentfillcolorspace==4)
		{//�����ҲͿĨ,Ҫ����������ɫ
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			getline->set[getline->count].color[1]=currentcolor[1];
			getline->set[getline->count].color[2]=currentcolor[2];
			*/
			currentstackpoint->currentroute->color[0]=currentfillcolor[0];
			currentstackpoint->currentroute->color[1]=currentfillcolor[1];
			currentstackpoint->currentroute->color[2]=currentfillcolor[2];
		}
		else
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			*/
			currentstackpoint->currentroute->color[0]=currentfillcolor[0];
		}
		currentstackpoint->currentroute->scolor=(float*)malloc(4*sizeof(float));
		//Ϊscolor����ռ�
		if(currentstrokecolorspace==3||currentstrokecolorspace==4)
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			getline->set[getline->count].color[1]=currentcolor[1];
			getline->set[getline->count].color[2]=currentcolor[2];
			*/
			currentstackpoint->currentroute->scolor[0]=currentstrokecolor[0];
			currentstackpoint->currentroute->scolor[1]=currentstrokecolor[1];
			currentstackpoint->currentroute->scolor[2]=currentstrokecolor[2];
		}
		else
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			*/
			currentstackpoint->currentroute->scolor[0]=currentfillcolor[0];
		}
		
	}
	//printf("\n��ǰ�ṹ�Ƿ�Ϊջ:%d\n",currentstackpoint->existstack);
	if(currentstackpoint->existstack!=0)
	{
		addroute(currentstackpoint);//������ջ�������·��  (��֮�����������һ������ĵ�����)
	}
	else//����Ϊ�����
	{
		currentstackpoint->countroute=1;//�����ֻ��һ��·��
		addstack(getline);
		currentstackpoint=getline->currentstack;
		getline->count++;
	}
}

static void pdf_run_c(pdf_csi *csi)
{
	float a, b, c, d, e, f;
	a = csi->stack[0];
	b = csi->stack[1];
	c = csi->stack[2];
	d = csi->stack[3];
	e = csi->stack[4];
	f = csi->stack[5];
#ifdef debug
	printf("���������߲��� %f %f %f %f %f %f \n",a,b,c,d,e,f);
#endif
	/*getline->set[getline->count].type=2;//����·������ 2-����������
	getline->set[getline->count].points[getline->set[getline->count].countpoint][0]=a;
	getline->set[getline->count].points[getline->set[getline->count].countpoint][1]=b;
	getline->set[getline->count].points[getline->set[getline->count].countpoint][2]=c;
	getline->set[getline->count].points[getline->set[getline->count].countpoint][3]=d;
	getline->set[getline->count].points[getline->set[getline->count].countpoint][4]=e;
	getline->set[getline->count].points[getline->set[getline->count].countpoint][5]=f;
	getline->set[getline->count].points[getline->set[getline->count].countpoint][6]=3;//��Ӧc
	getline->set[getline->count].countpoint++;
	*/
	currentstackpoint->currentroute->currentpoint->p0=a;
	currentstackpoint->currentroute->currentpoint->p1=b;
	currentstackpoint->currentroute->currentpoint->p2=c;
	currentstackpoint->currentroute->currentpoint->p3=d;
	currentstackpoint->currentroute->currentpoint->p4=e;
	currentstackpoint->currentroute->currentpoint->p5=f;
	currentstackpoint->currentroute->currentpoint->state=3;
	Vpointx=e;
	Vpointy=f;
	currentstackpoint->currentroute->countpoint++;
	addpoint(currentstackpoint->currentroute);
	fz_curveto(csi->dev->ctx, csi->path, a, b, c, d, e, f);
}

static void pdf_run_cm(pdf_csi *csi)//ָ����ǰ��ת������CTM
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	fz_matrix m;
	int i=0;

	m.a = csi->stack[0];
	m.b = csi->stack[1];
	m.c = csi->stack[2];
	m.d = csi->stack[3];
	m.e = csi->stack[4];
	m.f = csi->stack[5];
	//getline->set[getline->count].matrix=(float*)malloc(6);
	//getline->set[getline->count].existcm=1;
	currentstackpoint->matrix=(float*)malloc(6*(sizeof(float)));/*(float*)malloc(6);*/
	currentstackpoint->existcm=1;
	for (i=0;i<6;i++)
	{
		//getline->set[getline->count].matrix[i]=csi->stack[i];
		currentstackpoint->matrix[i]=csi->stack[i];
#ifdef debug
		printf("����ת�þ���:%f\n",currentstackpoint->matrix[i]);
#endif
	}
	//printf("����ת�þ���%f %f %f %f %f %f \n",getline->set[getline->count].matrix[0],getline->set[getline->count].matrix[1],getline->set[getline->count].matrix[2],getline->set[getline->count].matrix[3],getline->set[getline->count].matrix[4],getline->set[getline->count].matrix[5]);
	gstate->ctm = fz_concat(m, gstate->ctm);
}

static void pdf_run_d(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_obj *array;
	int i;
	int len;

	array = csi->obj;
	len = pdf_array_len(array);
	gstate->stroke_state = fz_unshare_stroke_state_with_len(csi->dev->ctx, gstate->stroke_state, len);
	gstate->stroke_state->dash_len = len;
	for (i = 0; i < len; i++)
		gstate->stroke_state->dash_list[i] = pdf_to_real(pdf_array_get(array, i));
	gstate->stroke_state->dash_phase = csi->stack[0];
}

static void pdf_run_d0(pdf_csi *csi)
{
	csi->dev->flags |= FZ_DEVFLAG_COLOR;
}

static void pdf_run_d1(pdf_csi *csi)
{
	csi->dev->flags |= FZ_DEVFLAG_MASK;
	csi->dev->flags &= ~(FZ_DEVFLAG_FILLCOLOR_UNDEFINED |
				FZ_DEVFLAG_STROKECOLOR_UNDEFINED |
				FZ_DEVFLAG_STARTCAP_UNDEFINED |
				FZ_DEVFLAG_DASHCAP_UNDEFINED |
				FZ_DEVFLAG_ENDCAP_UNDEFINED |
				FZ_DEVFLAG_LINEJOIN_UNDEFINED |
				FZ_DEVFLAG_MITERLIMIT_UNDEFINED |
				FZ_DEVFLAG_LINEWIDTH_UNDEFINED);
}

static void pdf_run_f(pdf_csi *csi)
{
	pdf_show_path(csi, 0, 1, 0, 0);
	currentstackpoint->currentroute->drawingmethord=3;
	currentstackpoint->countroute++;
	if(currentstackpoint->currentroute->colorspace==-1)//��ɫ�ʿռ䲻��,����˳��
	{
		//getline->set[getline->count].colorspace=currentcolorspace;
		currentstackpoint->currentroute->colorspace=currentfillcolorspace;
		//printf("����:currentcolorspace:  %d\n",currentcolorspace);
		//printf("����:˳��RGBɫ�ʿռ� %.3f %.3f %.3f\n",currentcolor[0],currentcolor[1],currentcolor[2]);
		if(currentfillcolorspace==3||currentfillcolorspace==4)
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			getline->set[getline->count].color[1]=currentcolor[1];
			getline->set[getline->count].color[2]=currentcolor[2];
			*/
			currentstackpoint->currentroute->color[0]=currentfillcolor[0];
			currentstackpoint->currentroute->color[1]=currentfillcolor[1];
			currentstackpoint->currentroute->color[2]=currentfillcolor[2];
		}
		else
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			*/
			currentstackpoint->currentroute->color[0]=currentfillcolor[0];
		}
		
	}
	//printf("\n��ǰ�ṹ�Ƿ�Ϊջ:%d\n",currentstackpoint->existstack);
	if(currentstackpoint->existstack!=0)
	{
		addroute(currentstackpoint);//������ջ�������·��  (��֮�����������һ������ĵ�����)
	}
	else//����Ϊ�����
	{
		currentstackpoint->countroute=1;//�����ֻ��һ��·��
		addstack(getline);
		currentstackpoint=getline->currentstack;
		getline->count++;
	}
}

static void pdf_run_fstar(pdf_csi *csi)
{
	pdf_show_path(csi, 0, 1, 0, 1);
	//getline->set[getline->count].drawingmethord=2;//���û��Ʒ��� 2-f*
	currentstackpoint->currentroute->drawingmethord=2;
	currentstackpoint->countroute++;
	if(currentstackpoint->currentroute->colorspace==-1)//��ɫ�ʿռ䲻��,����˳��
	{
		//getline->set[getline->count].colorspace=currentcolorspace;
		currentstackpoint->currentroute->colorspace=currentfillcolorspace;
		//printf("����:currentcolorspace:  %d\n",currentcolorspace);
		//printf("����:˳��RGBɫ�ʿռ� %.3f %.3f %.3f\n",currentcolor[0],currentcolor[1],currentcolor[2]);
		if(currentfillcolorspace==3||currentfillcolorspace==4)
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			getline->set[getline->count].color[1]=currentcolor[1];
			getline->set[getline->count].color[2]=currentcolor[2];
			*/
			currentstackpoint->currentroute->color[0]=currentfillcolor[0];
			currentstackpoint->currentroute->color[1]=currentfillcolor[1];
			currentstackpoint->currentroute->color[2]=currentfillcolor[2];
		}
		else
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			*/
			currentstackpoint->currentroute->color[0]=currentstrokecolor[0];
		}
		
	}
	//printf("\n��ǰ�ṹ�Ƿ�Ϊջ:%d\n",currentstackpoint->existstack);
	if(currentstackpoint->existstack!=0)
	{
		addroute(currentstackpoint);//������ջ�������·��  (��֮�����������һ������ĵ�����)
	}
	else//����Ϊ�����
	{
		currentstackpoint->countroute=1;//�����ֻ��һ��·��
		addstack(getline);
		currentstackpoint=getline->currentstack;
		getline->count++;
	}
	/*if(stackstate==0)//����ջ�ڵ�ʱ��
	{
		getline->count=getline->count+1;//·�����
		addstack(getline);
	}
	else
	{
		addroute(getline->currentstack);
		getline->currentstack->countroute++;
	}
	*/
	//getline->count=getline->count+1;
	//addroute(getline);//�½���һ��·��
	//printf("·��%d���",getline.count);
}

static void pdf_run_g(pdf_csi *csi)
{
	colorchanged=1;
	csi->dev->flags &= ~FZ_DEVFLAG_FILLCOLOR_UNDEFINED;
	pdf_set_colorspace(csi, PDF_FILL, fz_device_gray);
	pdf_set_color(csi, PDF_FILL, csi->stack);
	//getline->set[getline->count].colorspace=2;//������ɫ�ռ� 2����g
	//currentstackpoint->currentroute->colorspace=2;
	currentfillcolorspace=2;//���õ�ǰ��ɫ�ռ� �Ա�˳��
}

static void pdf_run_gs(pdf_csi *csi, pdf_obj *rdb)
{
	pdf_obj *dict;
	pdf_obj *obj;
	fz_context *ctx = csi->dev->ctx;

	dict = pdf_dict_gets(rdb, "ExtGState");
	if (!dict)
		fz_throw(ctx, "cannot find ExtGState dictionary");

	obj = pdf_dict_gets(dict, csi->name);
	if (!obj)
		fz_throw(ctx, "cannot find extgstate resource '%s'", csi->name);

	pdf_run_extgstate(csi, rdb, obj);//����ͼ��״̬�����ֵ�
	/* RJW: "cannot set ExtGState (%d 0 R)", pdf_to_num(obj) */
}

static void pdf_run_h(pdf_csi *csi)
{
	fz_closepath(csi->dev->ctx, csi->path);
	/*getline->set[getline->count].points[getline->set[getline->count].countpoint][0]=-1;
	getline->set[getline->count].points[getline->set[getline->count].countpoint][1]=-1;
	getline->set[getline->count].points[getline->set[getline->count].countpoint][2]=-1;
	getline->set[getline->count].points[getline->set[getline->count].countpoint][3]=-1;
	getline->set[getline->count].points[getline->set[getline->count].countpoint][4]=-1;
	getline->set[getline->count].points[getline->set[getline->count].countpoint][5]=-1;
	getline->set[getline->count].points[getline->set[getline->count].countpoint][6]=2;//�յ�,h�Ƿ������
	getline->set[getline->count].countpoint++;//��ĸ���++;
	*/
	currentstackpoint->currentroute->currentpoint->state=2;
	addpoint(currentstackpoint->currentroute);//����һ���µĵ�
	currentstackpoint->currentroute->countpoint++;//����++
}

static void pdf_run_i(pdf_csi *csi)
{
}

static void pdf_run_j(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	csi->dev->flags &= ~FZ_DEVFLAG_LINEJOIN_UNDEFINED;
	gstate->stroke_state = fz_unshare_stroke_state(csi->dev->ctx, gstate->stroke_state);
	gstate->stroke_state->linejoin = csi->stack[0];
#ifdef debug
	printf("��������ʽ:%f\n",csi->stack[0]);
#endif
}

static void pdf_run_k(pdf_csi *csi)
{
	colorchanged=1;
	csi->dev->flags &= ~FZ_DEVFLAG_FILLCOLOR_UNDEFINED;
	pdf_set_colorspace(csi, PDF_FILL, fz_device_cmyk);
	currentstackpoint->currentroute->colorspace=5;//cmykɫ�ʿռ�
	currentfillcolorspace=5;
	pdf_set_color(csi, PDF_FILL, csi->stack);//������ɫ
}

static void pdf_run_l(pdf_csi *csi)
{
	float a, b;
	a = csi->stack[0];
	b = csi->stack[1];
	/*
	getline->set[getline->count].type=1;//��ֱ��·�� 1
	getline->set[getline->count].points[getline->set[getline->count].countpoint][0]=a;
	getline->set[getline->count].points[getline->set[getline->count].countpoint][1]=b;
	getline->set[getline->count].points[getline->set[getline->count].countpoint][6]=1;
	getline->set[getline->count].countpoint++;//��ĸ���++;
	*/
	currentstackpoint->currentroute->currentpoint->p0=a;
	currentstackpoint->currentroute->currentpoint->p1=b;
	Vpointx=a;
	Vpointy=b;
	currentstackpoint->currentroute->currentpoint->state=1;
	currentstackpoint->currentroute->countpoint++;
	//getline->current->type=1;
	addpoint(currentstackpoint->currentroute);//����һ���µĵ�
	fz_lineto(csi->dev->ctx, csi->path, a, b);
}

static void pdf_run_m(pdf_csi *csi)
{
	float a, b;
	a = csi->stack[0];
	b = csi->stack[1];
	
	
	currentstackpoint->currentroute->currentpoint->p0=a;
	currentstackpoint->currentroute->currentpoint->p1=b;
	currentstackpoint->currentroute->currentpoint->state=0;
	currentstackpoint->currentroute->countpoint++;
	Vpointx=a;
	Vpointy=b;
	addpoint(currentstackpoint->currentroute);
#ifdef debug
	printf("�������%f %f \n",a,b);
#endif
	fz_moveto(csi->dev->ctx, csi->path, a, b);
}

static void pdf_run_n(pdf_csi *csi)
{
	pdf_show_path(csi, 0, 0, 0, 0);
}

static void pdf_run_q(pdf_csi *csi)
{
	stackstate++;
	
	if(stackstate==1)//�����
	{
		currentstackpoint->existstack=1;
		//currentstackpoint->existnest=0;
		//printf("����stack:����� %d\n",currentstackpoint->existnest);
		//addneststack(currentstackpoint);
		push(currentstackpoint);
	}
	else
	{
		currentstackpoint->existnest=1;
		//printf("����stack: %d\n",currentstackpoint->existnest);
		addneststack(currentstackpoint);
		currentstackpoint=currentstackpoint->currentstack;//��ǰջָ�������ƶ�һ��
		push(currentstackpoint);
		currentstackpoint->existstack=1;
	}
	pdf_gsave(csi);
}

static void pdf_run_re(pdf_csi *csi)
{
	fz_context *ctx = csi->dev->ctx;
	float x, y, w, h;

	x = csi->stack[0];
	y = csi->stack[1];
	w = csi->stack[2];
	h = csi->stack[3];
#ifdef debug
	printf("���β���:%f %f %f %f \n",x,y,w,h);
#endif
	/*
	getline->set[getline->count].points[0][0]=x;
	getline->set[getline->count].points[0][1]=y;
	getline->set[getline->count].points[0][6]=0;
	getline->set[getline->count].points[1][0]=x+w;
	getline->set[getline->count].points[1][1]=y;
	getline->set[getline->count].points[1][6]=1;
	getline->set[getline->count].points[2][0]=x+w;
	getline->set[getline->count].points[2][1]=y+h;
	getline->set[getline->count].points[2][6]=1;
	getline->set[getline->count].points[3][0]=x;
	getline->set[getline->count].points[3][1]=y+h;
	getline->set[getline->count].points[3][6]=1;
	getline->set[getline->count].points[4][0]=x;
	getline->set[getline->count].points[4][1]=y;
	getline->set[getline->count].points[4][6]=1;
	//���þ���,��m,l,l,l�ĸ�ʽ
	getline->set[getline->count].type=3;
	//��ʱ����·����������Ϊ����(������W��W*,���޸�Ϊ����·������) 3-����
	getline->set[getline->count].countpoint=5;
	//���ε���Ϊ5
	*/
	currentstackpoint->currentroute->currentpoint->p0=x;
	currentstackpoint->currentroute->currentpoint->p1=y;
	currentstackpoint->currentroute->currentpoint->state=0;
	addpoint(currentstackpoint->currentroute);
	currentstackpoint->currentroute->currentpoint->p0=x+w;
	currentstackpoint->currentroute->currentpoint->p1=y;
	currentstackpoint->currentroute->currentpoint->state=1;
	addpoint(currentstackpoint->currentroute);
	currentstackpoint->currentroute->currentpoint->p0=x+w;
	currentstackpoint->currentroute->currentpoint->p1=y+h;
	currentstackpoint->currentroute->currentpoint->state=1;
	addpoint(currentstackpoint->currentroute);
	currentstackpoint->currentroute->currentpoint->p0=x;
	currentstackpoint->currentroute->currentpoint->p1=y+h;
	currentstackpoint->currentroute->currentpoint->state=1;
	addpoint(currentstackpoint->currentroute);
	currentstackpoint->currentroute->currentpoint->p0=x;
	currentstackpoint->currentroute->currentpoint->p1=y;
	currentstackpoint->currentroute->currentpoint->state=2;
	currentstackpoint->currentroute->countpoint+=5;
	currentstackpoint->currentroute->type=3;
	addpoint(currentstackpoint->currentroute);
	fz_moveto(ctx, csi->path, x, y);
	fz_lineto(ctx, csi->path, x + w, y);
	fz_lineto(ctx, csi->path, x + w, y + h);
	fz_lineto(ctx, csi->path, x, y + h);
	fz_closepath(ctx, csi->path);
}

static void pdf_run_rg(pdf_csi *csi)
{
	colorchanged=1;
	csi->dev->flags &= ~FZ_DEVFLAG_FILLCOLOR_UNDEFINED;
	pdf_set_colorspace(csi, PDF_FILL, fz_device_rgb);
	pdf_set_color(csi, PDF_FILL, csi->stack);
	//getline->set[getline->count].colorspace=4;//ɫ�ʿռ� 4����rg
	//currentstackpoint->currentroute->colorspace=4;
	currentfillcolorspace=4;//���õ�ǰ��ɫ�ռ� �Ա�˳��
}

static void pdf_run_ri(pdf_csi *csi)
{
}

static void pdf_run(pdf_csi *csi)//s ��ӦS+h
{
	pdf_show_path(csi, 1, 0, 1, 0);
	currentstackpoint->currentroute->currentpoint->state=2;
	addpoint(currentstackpoint->currentroute);//����һ���µĵ�
	currentstackpoint->currentroute->countpoint++;
	currentstackpoint->currentroute->drawingmethord=4;
	currentstackpoint->countroute++;
	if(currentstackpoint->currentroute->colorspace==-1)
	{
		currentstackpoint->currentroute->colorspace=currentstrokecolorspace;
		//getline->set[getline->count].colorspace=currentcolorspace;
		//printf("����:currentcolorspace:  %d\n",currentcolorspace);
		//rintf("����:˳��RGBɫ�ʿռ� %.3f %.3f %.3f\n",currentcolor[0],currentcolor[1],currentcolor[2]);
		if(currentstrokecolorspace==3||currentstrokecolorspace==4)
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			getline->set[getline->count].color[1]=currentcolor[1];
			getline->set[getline->count].color[2]=currentcolor[2];
			*/
			currentstackpoint->currentroute->color[0]=currentstrokecolor[0];
			currentstackpoint->currentroute->color[1]=currentstrokecolor[1];
			currentstackpoint->currentroute->color[2]=currentstrokecolor[2];
		}
		else
		{
			/*
			getline->set[getline->count].color[0]=currentcolor[0];
			*/
			currentstackpoint->currentroute->color[0]=currentstrokecolor[0];
		}	
	}
	if(currentstackpoint->currentroute->linewidth==-1)//�߿�˳��
	{
		currentstackpoint->currentroute->linewidth=currentlinewidth;
	}
	if(currentstackpoint->existstack!=0)
	{
		addroute(currentstackpoint);//������ջ�������·��  (��֮�����������һ������ĵ�����)
	}
	else//����Ϊ�����
	{
		currentstackpoint->countroute=1;//�����ֻ��һ��·��
		addstack(getline);
		currentstackpoint=getline->currentstack;
		getline->count++;
	}
}

static void pdf_run_sh(pdf_csi *csi, pdf_obj *rdb)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_obj *dict;
	pdf_obj *obj;
	fz_shade *shd;

	dict = pdf_dict_gets(rdb, "Shading");
	if (!dict)
		fz_throw(ctx, "cannot find shading dictionary");

	obj = pdf_dict_gets(dict, csi->name);
	if (!obj)
		fz_throw(ctx, "cannot find shading resource: '%s'", csi->name);

	if ((csi->dev->hints & FZ_IGNORE_SHADE) == 0)
	{
		shd = pdf_load_shading(csi->xref, obj);//������Ӱ
		/* RJW: "cannot load shading (%d %d R)", pdf_to_num(obj), pdf_to_gen(obj) */
		fz_try(ctx)
		{
			pdf_show_shade(csi, shd);
		}
		fz_catch(ctx)
		{
			fz_drop_shade(ctx, shd);
			fz_rethrow(ctx);
		}
		fz_drop_shade(ctx, shd);
	}
}

static void pdf_run_v(pdf_csi *csi)
{
	float a, b, c, d;
	a = csi->stack[0];
	b = csi->stack[1];
	c = csi->stack[2];
	d = csi->stack[3];
	currentstackpoint->currentroute->currentpoint->p0=Vpointx;
	currentstackpoint->currentroute->currentpoint->p1=Vpointy;
	currentstackpoint->currentroute->currentpoint->p2=a;
	currentstackpoint->currentroute->currentpoint->p3=b;
	currentstackpoint->currentroute->currentpoint->p4=c;
	currentstackpoint->currentroute->currentpoint->p5=d;
	currentstackpoint->currentroute->currentpoint->state=3;
	Vpointx=c;
	Vpointy=d;
	currentstackpoint->currentroute->countpoint++;
	addpoint(currentstackpoint->currentroute);
	fz_curvetov(csi->dev->ctx, csi->path, a, b, c, d);
}

static void pdf_run_w(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_flush_text(csi); /* linewidth affects stroked text rendering mode */
	csi->dev->flags &= ~FZ_DEVFLAG_LINEWIDTH_UNDEFINED;
	gstate->stroke_state = fz_unshare_stroke_state(csi->dev->ctx, gstate->stroke_state);
	gstate->stroke_state->linewidth = csi->stack[0];
	//getline->set[getline->count].type=1;//����·������
	//getline->set[getline->count].linewidth= csi->stack[0];//�����߿�
	currentstackpoint->currentroute->linewidth=csi->stack[0];
	currentlinewidth=csi->stack[0];//˳���߿�
}

static void pdf_run_y(pdf_csi *csi)
{
	float a, b, c, d;
	a = csi->stack[0];
	b = csi->stack[1];
	c = csi->stack[2];
	d = csi->stack[3];
	currentstackpoint->currentroute->currentpoint->p0=a;
	currentstackpoint->currentroute->currentpoint->p1=b;
	currentstackpoint->currentroute->currentpoint->p2=c;
	currentstackpoint->currentroute->currentpoint->p3=d;
	currentstackpoint->currentroute->currentpoint->p4=c;
	currentstackpoint->currentroute->currentpoint->p5=d;
	currentstackpoint->currentroute->currentpoint->state=3;
	Vpointx=c;
	Vpointy=d;
	currentstackpoint->currentroute->countpoint++;
	addpoint(currentstackpoint->currentroute);
	fz_curvetoy(csi->dev->ctx, csi->path, a, b, c, d);
}

static void pdf_run_squote(pdf_csi *csi)
{
	fz_matrix m;
	pdf_gstate *gstate = csi->gstate + csi->gtop;

	m = fz_translate(0, -gstate->leading);
	csi->tlm = fz_concat(m, csi->tlm);
	csi->tm = csi->tlm;

	if (csi->string_len)
		pdf_show_string(csi, csi->string, csi->string_len);
	else
		pdf_show_text(csi, csi->obj);
}

static void pdf_run_dquote(pdf_csi *csi)
{
	fz_matrix m;
	pdf_gstate *gstate = csi->gstate + csi->gtop;

	gstate->word_space = csi->stack[0];
	gstate->char_space = csi->stack[1];

	m = fz_translate(0, -gstate->leading);
	csi->tlm = fz_concat(m, csi->tlm);
	csi->tm = csi->tlm;

	if (csi->string_len)
		pdf_show_string(csi, csi->string, csi->string_len);
	else
		pdf_show_text(csi, csi->obj);
}

#define A(a) (a)
#define B(a,b) (a | b << 8)
#define C(a,b,c) (a | b << 8 | c << 16)

static void
pdf_run_keyword(pdf_csi *csi, pdf_obj *rdb, fz_stream *file, char *buf)//���ݽ�������keywordִ�ж�Ӧ��PDF����
{
	fz_context *ctx = csi->dev->ctx;
	int key;
#ifdef debug
	printf("%s��   ",buf);//buf�����ŵ���ʶ�����keyword
#endif
	key = buf[0];
	if (buf[1])
	{
		key |= buf[1] << 8;
		if (buf[2])
		{
			key |= buf[2] << 16;
			if (buf[3])
				key = 0;
		}
	}

	switch (key)
	{
	case A('"'): pdf_run_dquote(csi); break;
	case A('\''): pdf_run_squote(csi); break;
	case A('B'): pdf_run_B(csi); break;//·������ ���Ȼ��ͿĨ·�� ʹ�÷��������ԭ��
	case B('B','*'): pdf_run_Bstar(csi); break;//·������ ���Ȼ��ͿĨ·�� ʹ����żԭ��
	case C('B','D','C'): pdf_run_BDC(csi, rdb); break;
	case B('B','I'):
		pdf_run_BI(csi, rdb, file);
		/* RJW: "cannot draw inline image" */
		break;
	case C('B','M','C'): pdf_run_BMC(csi); break;
	case B('B','T'): pdf_run_BT(csi); break;//�ı����� ��ʼһ���ı����󣬳�ʼ���ı�����Tm���ı��о���Tlm
	case B('B','X'): pdf_run_BX(csi); break;
	case B('C','S'): pdf_run_CS(csi, rdb); break;
	case B('D','P'): pdf_run_DP(csi); break;
	case B('D','o'):
		fz_try(ctx)
		{
			pdf_run_Do(csi, rdb);
		}
		fz_catch(ctx)
		{
			fz_warn(ctx, "cannot draw xobject/image");
		}
		break;
	case C('E','M','C'): pdf_run_EMC(csi); break;
	case B('E','T'): pdf_run_ET(csi); break;//�ı����� ���һ���ı����󣬽���һ���ı������²��в����ˣ�
	case B('E','X'): pdf_run_EX(csi); break;
	case A('F'): pdf_run_F(csi); break;//·������ �൱��f��Ϊ�˼����Դ���
	case A('G'): pdf_run_G(csi); break;//ɫ�ʿռ����� G�ռ�
	case A('J'): pdf_run_J(csi); break;//ͼ��״̬���� �����߶˿���ʽ
	case A('K'): pdf_run_K(csi); break;
	case A('M'): pdf_run_M(csi); break;//ͼ��״̬���� ����б�м���
	case B('M','P'): pdf_run_MP(csi); break;
	case A('Q'): pdf_run_Q(csi); break;//ͼ��״̬���� ɾ����ջ�����´洢��״̬����ԭͼ��״̬
	case B('R','G'): pdf_run_RG(csi); break;//ɫ�ʿռ����� RGB�ռ�
	case A('S'): pdf_run_S(csi); break;//·������ ͿĨ��·�����ƾ�����pdf_show_path������ֻ����������ͬ��Ч����ͬ��
	case B('S','C'): pdf_run_SC(csi, rdb); break;
	case C('S','C','N'): pdf_run_SC(csi, rdb); break;
	case B('T','*'): pdf_run_Tstar(csi); break;//�ı�λ�ò��� �ƶ�����һ���ײ�
	case B('T','D'): pdf_run_TD(csi); break;//�ı�λ�ò��� �ƶ�����һ�еĿ�ʼ
	case B('T','J'): pdf_run_TJ(csi); break;//�ı���ʾ����
	case B('T','L'): pdf_run_TL(csi); break;//�ı��о�
	case B('T','c'): pdf_run_Tc(csi); break;//�ı��ַ����
	case B('T','d'): pdf_run_Td(csi); break;//�ı�λ�ò��� �ƶ�����һ�еĿ�ʼ
	case B('T','f')://�ı�����
		fz_try(ctx)
		{
			pdf_run_Tf(csi, rdb);
		}
		fz_catch(ctx)
		{
			fz_warn(ctx, "cannot set font");
		}
		break;
	case B('T','j'): pdf_run_Tj(csi); break;//�ı���ʾ���� ��ʾ�ַ���
	case B('T','m'): pdf_run_Tm(csi); break;//�ı�λ�ò��� �����ı�����Tm���ı��о���Tlm
	case B('T','r'): pdf_run_Tr(csi); break;
	case B('T','s'): pdf_run_Ts(csi); break;
	case B('T','w'): pdf_run_Tw(csi); break;//�ı��ּ��
	case B('T','z'): pdf_run_Tz(csi); break;
	case A('W'): pdf_run_W(csi); break;//·������ ͨ��ʹ��ǰ·�������ཻ�޸ĵ�ǰ·����������������
	case B('W','*'): pdf_run_Wstar(csi); break;//·������ ͨ��ʹ��ǰ·�������ཻ�޸ĵ�ǰ·������ż����
	case A('b'): pdf_run_b(csi); break;//·������ �رգ���䣬��ͿĨ·����������ƹ���
	case B('b','*'): pdf_run_bstar(csi); break;//·������ �رգ���䣬��ͿĨ·������żԭ��
	case A('c'): pdf_run_c(csi); break;//·������ ��ǰ·������һ�����α���������
	case B('c','m'): pdf_run_cm(csi); break;//ͼ��״̬���� ָ��CTM����
	case B('c','s'): pdf_run_cs(csi, rdb); break;
	case A('d'): pdf_run_d(csi); break;//ͼ��״̬���� ��������ģʽ
	case B('d','0'): pdf_run_d0(csi); break;
	case B('d','1'): pdf_run_d1(csi); break;
	case A('f'): pdf_run_f(csi); break;//·������ ���·����ʹ�÷�����������������������
	case B('f','*'): pdf_run_fstar(csi); break;//·������ ʹ����ż���ԭ��
	case A('g'): pdf_run_g(csi); break;//ɫ�ʿռ����� g�ռ�
	case B('g','s')://ͼ��״̬���� ����ͼ��״̬��ָ���Ĳ���
		fz_try(ctx)
		{
			pdf_run_gs(csi, rdb);
		}
		fz_catch(ctx)
		{
			fz_warn(ctx, "cannot set graphics state");
		}
		break;
	case A('h'): pdf_run_h(csi); break;//·������ �ӵ�ǰ�㵽��·�����������һ��ֱ�߶Σ��ر���·��
	case A('i'): pdf_run_i(csi); break;//ͼ��״̬���� ����ƽ��ȹ���
	case A('j'): pdf_run_j(csi); break;//ͼ��״̬���� ������������ʽ
	case A('k'): pdf_run_k(csi); break;
	case A('l'): pdf_run_l(csi); break;//·������ �ӵ�ǰ����x��y��׷��һ��ֱ���߶Σ��µĵ�ǰ���޸�Ϊ��x��y��
	case A('m'): pdf_run_m(csi); break;//·������ ����ǰ���ƶ������꣨x��y������ʼһ���µ���·��
	case A('n'): pdf_run_n(csi); break;//·������ �����ͿĨ����ֱ�ӽ���·������
	case A('q'): pdf_run_q(csi); break;//ͼ��״̬���� ����ͼ��״̬��ջ�еĵ�ǰ����
	case B('r','e'): pdf_run_re(csi); break;//·������ ��ǰ·���м���һ��������Ϊ��յ���·��
	case B('r','g'): pdf_run_rg(csi); break;
	case B('r','i'): pdf_run_ri(csi); break;//ͼ��״̬���� ����ͼ����ʾ��ʽ
	case A('s'): pdf_run(csi); break;//·������ ͿĨ���ر�·��
	case B('s','c'): pdf_run_sc(csi, rdb); break;
	case C('s','c','n'): pdf_run_sc(csi, rdb); break;
	case B('s','h'):
		fz_try(ctx)
		{
			pdf_run_sh(csi, rdb);
		}
		fz_catch(ctx)
		{
			fz_warn(ctx, "cannot draw shading");
		}
		break;
	case A('v'): pdf_run_v(csi); break;//·������ ��ǰ·������һ�����α���������
	case A('w'): pdf_run_w(csi); break;//ͼ��״̬���� �����߿�
	case A('y'): pdf_run_y(csi); break;//·������ ��ǰ·������һ�����α���������
	default:
		if (!csi->xbalance)
			fz_warn(ctx, "unknown keyword: '%s'", buf);
		break;
	}
	fz_assert_lock_not_held(ctx, FZ_LOCK_FILE);
}

static void
pdf_run_stream(pdf_csi *csi, pdf_obj *rdb, fz_stream *file, pdf_lexbuf *buf)//�����������з��ಢ������Ӧ��������
{
	fz_context *ctx = csi->dev->ctx;
	int tok, in_array;
	stackstate=0;//Ĭ�ϲ���ͼ��״̬ջ��
	getline=(struct zblrouteset*)malloc(sizeof(struct zblrouteset));//ȫ�ֱ����ṹ��ָ�����ռ�
	//��ʼ��
	initcstack();
	initrouteset(getline);
	addstack(getline);
	currentstackpoint=getline->currentstack;//��ǰջָ��Ϊgetline��stackheadler->next,����һ���һ��ջ
/*
	for(i=0;i<1000;i++)
	{
		getline->set[i].countpoint=0;//������ʼ��
		getline->set[i].colorspace=-1;//ɫ�ʿռ��ʼ��
		getline->set[i].color[0]=-1;//��ɫ��ʼ��
		getline->set[i].color[1]=-1;
		getline->set[i].color[2]=-1;
		getline->set[i].color[3]=-1;
		getline->set[i].linewidth=-1;//�߿��ʼ��
		getline->set[i].drawingmethord=-1;//���Ʒ�����ʼ��
		for (j=0;j<50;j++)
			for(k=0;k<7;k++)
			{
				getline->set[i].points[j][k]=-1;
			}
		getline->set[i].existcm=0;//Ĭ�ϲ�����ת�þ���
		getline->set[i].matrix=NULL;//ת�þ���ָ��Ĭ��Ϊ��
	}
*/
	
	/* make sure we have a clean slate if we come here from flush_text */
	pdf_clear_stack(csi);//�����ջ
	in_array = 0;

	if (csi->cookie)
	{
		csi->cookie->progress_max = -1;
		csi->cookie->progress = 0;
	}

	while (1)
	{
		if (csi->top == nelem(csi->stack) - 1)
			fz_throw(ctx, "stack overflow");//��ջ���

		/* Check the cookie */
		if (csi->cookie)
		{
			if (csi->cookie->abort)
				break;
			csi->cookie->progress++;
		}

		tok = pdf_lex(file, buf);//pdf_lex�������з�����������ݴ����ж�ȡ�ķ������ݽ������ݷֳɲ�ͬ�����tok��Ϊ����ֵ�����ݲ�ͬ�������ݣ����ز�ͬ������
		

		if (in_array)//�Ƿ���������
		{
			if (tok == PDF_TOK_CLOSE_ARRAY)
			{
				in_array = 0;
			}
			else if (tok == PDF_TOK_REAL)//ʵ��
			{
				pdf_gstate *gstate = csi->gstate + csi->gtop;
				pdf_show_space(csi, -buf->f * gstate->size * 0.001f);
			}
			else if (tok == PDF_TOK_INT)//����
			{
				pdf_gstate *gstate = csi->gstate + csi->gtop;
				pdf_show_space(csi, -buf->i * gstate->size * 0.001f);
			}
			else if (tok == PDF_TOK_STRING)//�ַ���
			{
				pdf_show_string(csi, (unsigned char *)buf->scratch, buf->len);
			}
			else if (tok == PDF_TOK_KEYWORD)//pdf����
			{
				if (!strcmp(buf->scratch, "Tw") || !strcmp(buf->scratch, "Tc"))//Tc��Tw���ֱ����ַ���࣬�ּ��
					fz_warn(ctx, "ignoring keyword '%s' inside array", buf->scratch);//���������е�PDF����
				else
					fz_throw(ctx, "syntax error in array");
			}
			else if (tok == PDF_TOK_EOF)//�ļ���β
				return;
			else
				fz_throw(ctx, "syntax error in array");
		}

		else switch (tok)
		{
		case PDF_TOK_ENDSTREAM://����β
		case PDF_TOK_EOF:
			return;

		case PDF_TOK_OPEN_ARRAY://���鿪ʼ
			if (!csi->in_text)
			{
				csi->obj = pdf_parse_array(csi->xref, file, buf);//��������
				/* RJW: "cannot parse array" */
			}
			else
			{
				in_array = 1;
			}
			break;

		case PDF_TOK_OPEN_DICT:
			csi->obj = pdf_parse_dict(csi->xref, file, buf);//�����ֵ�
			/* RJW: "cannot parse dictionary" */
			break;

		case PDF_TOK_NAME:
			fz_strlcpy(csi->name, buf->scratch, sizeof(csi->name));//����name���ͱ���
			//printf("%s\n",csi->name);
			break;

		case PDF_TOK_INT:
			csi->stack[csi->top] = buf->i;
			csi->top ++;
			break;

		case PDF_TOK_REAL:
			csi->stack[csi->top] = buf->f;
			csi->top ++;
			break;

		case PDF_TOK_STRING://����String���ͱ���
			if (buf->len <= sizeof(csi->string))
			{
				memcpy(csi->string, buf->scratch, buf->len);
#ifdef debug
				printf("%s\n",csi->string);
#endif
				csi->string_len = buf->len;
			}
			else
			{
				csi->obj = pdf_new_string(ctx, buf->scratch, buf->len);
			}
			break;

		case PDF_TOK_KEYWORD:
			
			//printf("�����ⲿָ��ֵ:%p\n",getline);
			pdf_run_keyword(csi, rdb, file, buf->scratch);
			//OutputDebugString(L"===========keyword============");
			/* RJW: "cannot run keyword" */
			pdf_clear_stack(csi);
			break;

		default:
			fz_throw(ctx, "syntax error in content stream");
		}
	}
}

/*
 * Entry points
 */

static void
pdf_run_buffer(pdf_csi *csi, pdf_obj *rdb, fz_buffer *contents)
{
	fz_context *ctx = csi->dev->ctx;
	pdf_lexbuf_large *buf;
	fz_stream * file = NULL;
	int save_in_text;

	fz_var(buf);
	fz_var(file);

	if (contents == NULL)
		return;

	fz_try(ctx)
	{
		buf = fz_malloc(ctx, sizeof(*buf)); /* we must be re-entrant for type3 fonts ���Ǳ���ΪType3��������*/
		buf->base.size = PDF_LEXBUF_LARGE;
		file = fz_open_buffer(ctx, contents);
		save_in_text = csi->in_text;
		csi->in_text = 0;
		fz_try(ctx)
		{
			pdf_run_stream(csi, rdb, file, &buf->base);//����������
		}
		fz_catch(ctx)
		{
			fz_warn(ctx, "Content stream parsing error - rendering truncated");//��������������-���ֽض�
		}
		csi->in_text = save_in_text;
	}
	fz_always(ctx)
	{
		fz_close(file);
		fz_free(ctx, buf);
	}
	fz_catch(ctx)
	{
		fz_throw(ctx, "cannot parse context stream");
	}
}

void
pdf_run_page_with_usage(pdf_document *xref, pdf_page *page, fz_device *dev, fz_matrix ctm, char *event, fz_cookie *cookie)
{
	fz_context *ctx = dev->ctx;
	pdf_csi *csi;
	pdf_annot *annot;
	int flags;

	ctm = fz_concat(page->ctm, ctm);//ת������


	if (page->transparency)//͸��ҳ��
		fz_begin_group(dev, fz_transform_rect(ctm, page->mediabox), 1, 0, 0, 1);

	csi = pdf_new_csi(xref, dev, ctm, event, cookie, NULL);
	fz_try(ctx)
	{
		pdf_run_buffer(csi, page->resources, page->contents);//����ҳ��contents
	}
	fz_catch(ctx)
	{
		pdf_free_csi(csi);
		fz_throw(ctx, "cannot parse page content stream");//�޷�����ҳ��������
	}
	pdf_free_csi(csi);

	if (cookie && cookie->progress_max != -1)
	{
		int count = 1;
		for (annot = page->annots; annot; annot = annot->next)
			count++;
		cookie->progress_max += count;
	}

	for (annot = page->annots; annot; annot = annot->next)
	{
		/* Check the cookie for aborting */
		if (cookie)
		{
			if (cookie->abort)
				break;
			cookie->progress++;
		}

		flags = pdf_to_int(pdf_dict_gets(annot->obj, "F"));

		/* TODO: NoZoom and NoRotate */
		if (flags & (1 << 0)) /* Invisible */
			continue;
		if (flags & (1 << 1)) /* Hidden */
			continue;
		if (!strcmp(event, "Print") && !(flags & (1 << 2))) /* Print */
			continue;
		if (!strcmp(event, "View") && (flags & (1 << 5))) /* NoView */
			continue;

		csi = pdf_new_csi(xref, dev, ctm, event, cookie, NULL);
		if (!pdf_is_hidden_ocg(pdf_dict_gets(annot->obj, "OC"), csi, page->resources))
		{
			fz_try(ctx)
			{
				pdf_run_xobject(csi, page->resources, annot->ap, annot->matrix);
			}
			fz_catch(ctx)
			{
				pdf_free_csi(csi);
				fz_throw(ctx, "cannot parse annotation appearance stream");
			}
		}
		pdf_free_csi(csi);
	}

	if (page->transparency)
		fz_end_group(dev);
}

void
pdf_run_page(pdf_document *xref, pdf_page *page, fz_device *dev, fz_matrix ctm, fz_cookie *cookie)
{
	//OutputDebugString(L"����pdf_run_page����");
	pdf_run_page_with_usage(xref, page, dev, ctm, "View", cookie);
}

void
pdf_run_glyph(pdf_document *xref, pdf_obj *resources, fz_buffer *contents, fz_device *dev, fz_matrix ctm, void *gstate)
{
	pdf_csi *csi = pdf_new_csi(xref, dev, ctm, "View", NULL, gstate);
	fz_context *ctx = xref->ctx;

	fz_try(ctx)
	{
		pdf_run_buffer(csi, resources, contents);
	}
	fz_catch(ctx)
	{
		pdf_free_csi(csi);
		fz_throw(ctx, "cannot parse glyph content stream");
	}
	pdf_free_csi(csi);
}


