/*
 * Copyright (c) 2000-2005 Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software Foundation,
 * Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef __XFS_DQUOT_H__
#define __XFS_DQUOT_H__


struct xfs_mount;
struct xfs_trans;

typedef struct xfs_dquot {
	uint		 dq_flags;	
	struct list_head q_lru;		
	struct xfs_mount*q_mount;	
	struct xfs_trans*q_transp;	
	uint		 q_nrefs;	
	xfs_daddr_t	 q_blkno;	
	int		 q_bufoffset;	
	xfs_fileoff_t	 q_fileoffset;	

	struct xfs_dquot*q_gdquot;	
	xfs_disk_dquot_t q_core;	
	xfs_dq_logitem_t q_logitem;	
	xfs_qcnt_t	 q_res_bcount;	
	xfs_qcnt_t	 q_res_icount;	
	xfs_qcnt_t	 q_res_rtbcount;
	struct mutex	 q_qlock;	
	struct completion q_flush;	
	atomic_t          q_pincount;	
	wait_queue_head_t q_pinwait;	
} xfs_dquot_t;

enum {
	XFS_QLOCK_NORMAL = 0,
	XFS_QLOCK_NESTED,
};

static inline void xfs_dqflock(xfs_dquot_t *dqp)
{
	wait_for_completion(&dqp->q_flush);
}

static inline int xfs_dqflock_nowait(xfs_dquot_t *dqp)
{
	return try_wait_for_completion(&dqp->q_flush);
}

static inline void xfs_dqfunlock(xfs_dquot_t *dqp)
{
	complete(&dqp->q_flush);
}

static inline int xfs_dqlock_nowait(struct xfs_dquot *dqp)
{
	return mutex_trylock(&dqp->q_qlock);
}

static inline void xfs_dqlock(struct xfs_dquot *dqp)
{
	mutex_lock(&dqp->q_qlock);
}

static inline void xfs_dqunlock(struct xfs_dquot *dqp)
{
	mutex_unlock(&dqp->q_qlock);
}

static inline int xfs_this_quota_on(struct xfs_mount *mp, int type)
{
	switch (type & XFS_DQ_ALLTYPES) {
	case XFS_DQ_USER:
		return XFS_IS_UQUOTA_ON(mp);
	case XFS_DQ_GROUP:
	case XFS_DQ_PROJ:
		return XFS_IS_OQUOTA_ON(mp);
	default:
		return 0;
	}
}

static inline xfs_dquot_t *xfs_inode_dquot(struct xfs_inode *ip, int type)
{
	switch (type & XFS_DQ_ALLTYPES) {
	case XFS_DQ_USER:
		return ip->i_udquot;
	case XFS_DQ_GROUP:
	case XFS_DQ_PROJ:
		return ip->i_gdquot;
	default:
		return NULL;
	}
}

#define XFS_DQ_IS_LOCKED(dqp)	(mutex_is_locked(&((dqp)->q_qlock)))
#define XFS_DQ_IS_DIRTY(dqp)	((dqp)->dq_flags & XFS_DQ_DIRTY)
#define XFS_QM_ISUDQ(dqp)	((dqp)->dq_flags & XFS_DQ_USER)
#define XFS_QM_ISPDQ(dqp)	((dqp)->dq_flags & XFS_DQ_PROJ)
#define XFS_QM_ISGDQ(dqp)	((dqp)->dq_flags & XFS_DQ_GROUP)
#define XFS_DQ_TO_QINF(dqp)	((dqp)->q_mount->m_quotainfo)
#define XFS_DQ_TO_QIP(dqp)	(XFS_QM_ISUDQ(dqp) ? \
				 XFS_DQ_TO_QINF(dqp)->qi_uquotaip : \
				 XFS_DQ_TO_QINF(dqp)->qi_gquotaip)

extern int		xfs_qm_dqread(struct xfs_mount *, xfs_dqid_t, uint,
					uint, struct xfs_dquot	**);
extern void		xfs_qm_dqdestroy(xfs_dquot_t *);
extern int		xfs_qm_dqflush(xfs_dquot_t *, uint);
extern void		xfs_qm_dqunpin_wait(xfs_dquot_t *);
extern void		xfs_qm_adjust_dqtimers(xfs_mount_t *,
					xfs_disk_dquot_t *);
extern void		xfs_qm_adjust_dqlimits(xfs_mount_t *,
					xfs_disk_dquot_t *);
extern int		xfs_qm_dqget(xfs_mount_t *, xfs_inode_t *,
					xfs_dqid_t, uint, uint, xfs_dquot_t **);
extern void		xfs_qm_dqput(xfs_dquot_t *);

extern void		xfs_dqlock2(struct xfs_dquot *, struct xfs_dquot *);
extern void		xfs_dqflock_pushbuf_wait(struct xfs_dquot *dqp);

static inline struct xfs_dquot *xfs_qm_dqhold(struct xfs_dquot *dqp)
{
	xfs_dqlock(dqp);
	dqp->q_nrefs++;
	xfs_dqunlock(dqp);
	return dqp;
}

#endif 
