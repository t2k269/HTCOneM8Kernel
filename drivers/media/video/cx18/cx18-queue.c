/*
 *  cx18 buffer queues
 *
 *  Derived from ivtv-queue.c
 *
 *  Copyright (C) 2007  Hans Verkuil <hverkuil@xs4all.nl>
 *  Copyright (C) 2008  Andy Walls <awalls@md.metrocast.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA
 */

#include "cx18-driver.h"
#include "cx18-queue.h"
#include "cx18-streams.h"
#include "cx18-scb.h"
#include "cx18-io.h"

void cx18_buf_swap(struct cx18_buffer *buf)
{
	int i;

	for (i = 0; i < buf->bytesused; i += 4)
		swab32s((u32 *)(buf->buf + i));
}

void _cx18_mdl_swap(struct cx18_mdl *mdl)
{
	struct cx18_buffer *buf;

	list_for_each_entry(buf, &mdl->buf_list, list) {
		if (buf->bytesused == 0)
			break;
		cx18_buf_swap(buf);
	}
}

void cx18_queue_init(struct cx18_queue *q)
{
	INIT_LIST_HEAD(&q->list);
	atomic_set(&q->depth, 0);
	q->bytesused = 0;
}

struct cx18_queue *_cx18_enqueue(struct cx18_stream *s, struct cx18_mdl *mdl,
				 struct cx18_queue *q, int to_front)
{
	
	if (q != &s->q_full) {
		mdl->bytesused = 0;
		mdl->readpos = 0;
		mdl->m_flags = 0;
		mdl->skipped = 0;
		mdl->curr_buf = NULL;
	}

	
	if (q == &s->q_busy &&
	    atomic_read(&q->depth) >= CX18_MAX_FW_MDLS_PER_STREAM)
		q = &s->q_free;

	spin_lock(&q->lock);

	if (to_front)
		list_add(&mdl->list, &q->list); 
	else
		list_add_tail(&mdl->list, &q->list); 
	q->bytesused += mdl->bytesused - mdl->readpos;
	atomic_inc(&q->depth);

	spin_unlock(&q->lock);
	return q;
}

struct cx18_mdl *cx18_dequeue(struct cx18_stream *s, struct cx18_queue *q)
{
	struct cx18_mdl *mdl = NULL;

	spin_lock(&q->lock);
	if (!list_empty(&q->list)) {
		mdl = list_first_entry(&q->list, struct cx18_mdl, list);
		list_del_init(&mdl->list);
		q->bytesused -= mdl->bytesused - mdl->readpos;
		mdl->skipped = 0;
		atomic_dec(&q->depth);
	}
	spin_unlock(&q->lock);
	return mdl;
}

static void _cx18_mdl_update_bufs_for_cpu(struct cx18_stream *s,
					  struct cx18_mdl *mdl)
{
	struct cx18_buffer *buf;
	u32 buf_size = s->buf_size;
	u32 bytesused = mdl->bytesused;

	list_for_each_entry(buf, &mdl->buf_list, list) {
		buf->readpos = 0;
		if (bytesused >= buf_size) {
			buf->bytesused = buf_size;
			bytesused -= buf_size;
		} else {
			buf->bytesused = bytesused;
			bytesused = 0;
		}
		cx18_buf_sync_for_cpu(s, buf);
	}
}

static inline void cx18_mdl_update_bufs_for_cpu(struct cx18_stream *s,
						struct cx18_mdl *mdl)
{
	struct cx18_buffer *buf;

	if (list_is_singular(&mdl->buf_list)) {
		buf = list_first_entry(&mdl->buf_list, struct cx18_buffer,
				       list);
		buf->bytesused = mdl->bytesused;
		buf->readpos = 0;
		cx18_buf_sync_for_cpu(s, buf);
	} else {
		_cx18_mdl_update_bufs_for_cpu(s, mdl);
	}
}

struct cx18_mdl *cx18_queue_get_mdl(struct cx18_stream *s, u32 id,
	u32 bytesused)
{
	struct cx18 *cx = s->cx;
	struct cx18_mdl *mdl;
	struct cx18_mdl *tmp;
	struct cx18_mdl *ret = NULL;
	LIST_HEAD(sweep_up);

	spin_lock(&s->q_busy.lock);
	list_for_each_entry_safe(mdl, tmp, &s->q_busy.list, list) {
		if (mdl->id != id) {
			mdl->skipped++;
			if (mdl->skipped >= atomic_read(&s->q_busy.depth)-1) {
				
				CX18_WARN("Skipped %s, MDL %d, %d "
					  "times - it must have dropped out of "
					  "rotation\n", s->name, mdl->id,
					  mdl->skipped);
				
				list_move_tail(&mdl->list, &sweep_up);
				atomic_dec(&s->q_busy.depth);
			}
			continue;
		}
		list_del_init(&mdl->list);
		atomic_dec(&s->q_busy.depth);
		ret = mdl;
		break;
	}
	spin_unlock(&s->q_busy.lock);

	if (ret != NULL) {
		ret->bytesused = bytesused;
		ret->skipped = 0;
		
		cx18_mdl_update_bufs_for_cpu(s, ret);
		if (s->type != CX18_ENC_STREAM_TYPE_TS)
			set_bit(CX18_F_M_NEED_SWAP, &ret->m_flags);
	}

	
	list_for_each_entry_safe(mdl, tmp, &sweep_up, list) {
		list_del_init(&mdl->list);
		cx18_enqueue(s, mdl, &s->q_free);
	}
	return ret;
}

static void cx18_queue_flush(struct cx18_stream *s,
			     struct cx18_queue *q_src, struct cx18_queue *q_dst)
{
	struct cx18_mdl *mdl;

	
	if (q_src == q_dst || q_dst == &s->q_full || q_dst == &s->q_busy)
		return;

	spin_lock(&q_src->lock);
	spin_lock(&q_dst->lock);
	while (!list_empty(&q_src->list)) {
		mdl = list_first_entry(&q_src->list, struct cx18_mdl, list);
		list_move_tail(&mdl->list, &q_dst->list);
		mdl->bytesused = 0;
		mdl->readpos = 0;
		mdl->m_flags = 0;
		mdl->skipped = 0;
		mdl->curr_buf = NULL;
		atomic_inc(&q_dst->depth);
	}
	cx18_queue_init(q_src);
	spin_unlock(&q_src->lock);
	spin_unlock(&q_dst->lock);
}

void cx18_flush_queues(struct cx18_stream *s)
{
	cx18_queue_flush(s, &s->q_busy, &s->q_free);
	cx18_queue_flush(s, &s->q_full, &s->q_free);
}

void cx18_unload_queues(struct cx18_stream *s)
{
	struct cx18_queue *q_idle = &s->q_idle;
	struct cx18_mdl *mdl;
	struct cx18_buffer *buf;

	
	cx18_queue_flush(s, &s->q_busy, q_idle);
	cx18_queue_flush(s, &s->q_full, q_idle);
	cx18_queue_flush(s, &s->q_free, q_idle);

	
	spin_lock(&q_idle->lock);
	list_for_each_entry(mdl, &q_idle->list, list) {
		while (!list_empty(&mdl->buf_list)) {
			buf = list_first_entry(&mdl->buf_list,
					       struct cx18_buffer, list);
			list_move_tail(&buf->list, &s->buf_pool);
			buf->bytesused = 0;
			buf->readpos = 0;
		}
		mdl->id = s->mdl_base_idx; 
		
	}
	spin_unlock(&q_idle->lock);
}

void cx18_load_queues(struct cx18_stream *s)
{
	struct cx18 *cx = s->cx;
	struct cx18_mdl *mdl;
	struct cx18_buffer *buf;
	int mdl_id;
	int i;
	u32 partial_buf_size;

	mdl_id = s->mdl_base_idx;
	for (mdl = cx18_dequeue(s, &s->q_idle), i = s->bufs_per_mdl;
	     mdl != NULL && i == s->bufs_per_mdl;
	     mdl = cx18_dequeue(s, &s->q_idle)) {

		mdl->id = mdl_id;

		for (i = 0; i < s->bufs_per_mdl; i++) {
			if (list_empty(&s->buf_pool))
				break;

			buf = list_first_entry(&s->buf_pool, struct cx18_buffer,
					       list);
			list_move_tail(&buf->list, &mdl->buf_list);

			
			cx18_writel(cx, buf->dma_handle,
				    &cx->scb->cpu_mdl[mdl_id + i].paddr);
			cx18_writel(cx, s->buf_size,
				    &cx->scb->cpu_mdl[mdl_id + i].length);
		}

		if (i == s->bufs_per_mdl) {
			partial_buf_size = s->mdl_size % s->buf_size;
			if (partial_buf_size) {
				cx18_writel(cx, partial_buf_size,
				      &cx->scb->cpu_mdl[mdl_id + i - 1].length);
			}
			cx18_enqueue(s, mdl, &s->q_free);
		} else {
			
			cx18_push(s, mdl, &s->q_idle);
		}
		mdl_id += i;
	}
}

void _cx18_mdl_sync_for_device(struct cx18_stream *s, struct cx18_mdl *mdl)
{
	int dma = s->dma;
	u32 buf_size = s->buf_size;
	struct pci_dev *pci_dev = s->cx->pci_dev;
	struct cx18_buffer *buf;

	list_for_each_entry(buf, &mdl->buf_list, list)
		pci_dma_sync_single_for_device(pci_dev, buf->dma_handle,
					       buf_size, dma);
}

int cx18_stream_alloc(struct cx18_stream *s)
{
	struct cx18 *cx = s->cx;
	int i;

	if (s->buffers == 0)
		return 0;

	CX18_DEBUG_INFO("Allocate %s stream: %d x %d buffers "
			"(%d.%02d kB total)\n",
		s->name, s->buffers, s->buf_size,
		s->buffers * s->buf_size / 1024,
		(s->buffers * s->buf_size * 100 / 1024) % 100);

	if (((char __iomem *)&cx->scb->cpu_mdl[cx->free_mdl_idx + s->buffers] -
				(char __iomem *)cx->scb) > SCB_RESERVED_SIZE) {
		unsigned bufsz = (((char __iomem *)cx->scb) + SCB_RESERVED_SIZE -
					((char __iomem *)cx->scb->cpu_mdl));

		CX18_ERR("Too many buffers, cannot fit in SCB area\n");
		CX18_ERR("Max buffers = %zd\n",
			bufsz / sizeof(struct cx18_mdl_ent));
		return -ENOMEM;
	}

	s->mdl_base_idx = cx->free_mdl_idx;

	
	for (i = 0; i < s->buffers; i++) {
		struct cx18_mdl *mdl;
		struct cx18_buffer *buf;

		
		mdl = kzalloc(sizeof(struct cx18_mdl), GFP_KERNEL|__GFP_NOWARN);
		if (mdl == NULL)
			break;

		buf = kzalloc(sizeof(struct cx18_buffer),
				GFP_KERNEL|__GFP_NOWARN);
		if (buf == NULL) {
			kfree(mdl);
			break;
		}

		buf->buf = kmalloc(s->buf_size, GFP_KERNEL|__GFP_NOWARN);
		if (buf->buf == NULL) {
			kfree(mdl);
			kfree(buf);
			break;
		}

		INIT_LIST_HEAD(&mdl->list);
		INIT_LIST_HEAD(&mdl->buf_list);
		mdl->id = s->mdl_base_idx; 
		cx18_enqueue(s, mdl, &s->q_idle);

		INIT_LIST_HEAD(&buf->list);
		buf->dma_handle = pci_map_single(s->cx->pci_dev,
				buf->buf, s->buf_size, s->dma);
		cx18_buf_sync_for_cpu(s, buf);
		list_add_tail(&buf->list, &s->buf_pool);
	}
	if (i == s->buffers) {
		cx->free_mdl_idx += s->buffers;
		return 0;
	}
	CX18_ERR("Couldn't allocate buffers for %s stream\n", s->name);
	cx18_stream_free(s);
	return -ENOMEM;
}

void cx18_stream_free(struct cx18_stream *s)
{
	struct cx18_mdl *mdl;
	struct cx18_buffer *buf;
	struct cx18 *cx = s->cx;

	CX18_DEBUG_INFO("Deallocating buffers for %s stream\n", s->name);

	
	cx18_unload_queues(s);

	
	while ((mdl = cx18_dequeue(s, &s->q_idle)))
		kfree(mdl);

	
	while (!list_empty(&s->buf_pool)) {
		buf = list_first_entry(&s->buf_pool, struct cx18_buffer, list);
		list_del_init(&buf->list);

		pci_unmap_single(s->cx->pci_dev, buf->dma_handle,
				s->buf_size, s->dma);
		kfree(buf->buf);
		kfree(buf);
	}
}
