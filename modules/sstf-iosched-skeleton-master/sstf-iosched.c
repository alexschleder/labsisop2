
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

long long unsigned int prevPos = 0;

/* SSTF data structure. */
struct sstf_data {
	struct list_head queue;
};

static void sstf_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

/* Esta função despacha o próximo bloco a ser lido. */
static int sstf_dispatch(struct request_queue *q, int force){
	struct sstf_data *nd = q->elevator->elevator_data;
	char direction = 'R';
	struct request *rq;
	rq = list_first_entry_or_null(&nd->queue, struct request, queuelist);

	/* Aqui deve-se retirar uma requisição da fila e enviá-la para processamento.
	 * Use como exemplo o driver noop-iosched.c. Veja como a requisição é tratada.
	 *
	 * Antes de retornar da função, imprima o sector que foi atendido.
	 */

	printk(KERN_EMERG "\nINICIO DISP:\nprevPos = %llu", prevPos);

	struct request *melhorItem = rq;
	long long unsigned int menorDif = 18000000000000000;

	int escolheu = 0;

	printk(KERN_EMERG "   Lista Diff:");
	// Ponteiro para nodo
	struct list_head *ptr;
	// Itera sobre lista da fila
	list_for_each(ptr, &nd->queue) {
		//printk(KERN_EMERG "   entrou no for");

		rq = list_entry(ptr, struct request, queuelist);


		printk(KERN_EMERG "       %llu", abs(prevPos - blk_rq_pos(rq)));
		// Nova Tentativa
		if(abs(prevPos - blk_rq_pos(rq)) < menorDif) {
			melhorItem = rq;
			menorDif = abs(prevPos - blk_rq_pos(rq));

			escolheu = 1;
		}
	}

	printk(KERN_EMERG "   Escolhido Diff %llu", menorDif);
	if(escolheu) {
		rq = melhorItem;
	}

	//rq = list_first_entry_or_null(&nd->queue, struct request, queuelist);
	if (rq) {
		printk(KERN_EMERG "-> retorna o rq");

		prevPos = blk_rq_pos(rq);

		list_del_init(&rq->queuelist);

		elv_dispatch_sort(q, rq);

		printk(KERN_EMERG "[SSTF]\033[0;36m dsp\033[0m %c %llu\n", direction, blk_rq_pos(rq));

		return 1;
	}
	printk(KERN_EMERG "-> sem dispatch");

	return 0;
}

static void sstf_add_request(struct request_queue *q, struct request *rq){
	struct sstf_data *nd = q->elevator->elevator_data;
	char direction = 'R';

	struct list_head *ptr;
	struct request *atual;

	printk(KERN_EMERG "\n ===== FILA TODA =====");
	list_for_each(ptr, &nd->queue) {
		atual = list_entry(ptr, struct request, queuelist);
		if(atual == list_last_entry(&nd->queue, struct request, queuelist)) {
			printk(KERN_EMERG "%llu",blk_rq_pos(atual));	
		} else {
			printk(KERN_EMERG "%llu >",blk_rq_pos(atual));
		}
	}

	int adicionado = 0;

	if (list_empty(&nd->queue))
	{
		list_add_tail(&rq->queuelist, &nd->queue);
		adicionado = 1;
		printk(KERN_EMERG "\n LISTA VAZIA ADD NO COMECO");

	}
	else
	{
		list_for_each(ptr, &nd->queue) 
		{
			atual = list_entry(ptr, struct request, queuelist);
			if (blk_rq_pos(atual) > blk_rq_pos(rq))
			{
				list_add_tail(&rq->queuelist, &atual->queuelist);
				printk(KERN_EMERG "\n FOI NO FOREACH");
				adicionado = 1;
				break;
			}
		}
	}

	if (!adicionado)
	{
		list_add(&rq->queuelist, &(list_last_entry(&nd->queue, struct request, queuelist))->queuelist);
	}

	printk(KERN_EMERG "[SSTF] \033[1;32madd\033[0m %c %llu\n", direction, blk_rq_pos(rq));	
}

static int sstf_init_queue(struct request_queue *q, struct elevator_type *e){
	struct sstf_data *nd;
	struct elevator_queue *eq;

	/* Implementação da inicialização da fila (queue).
	 *
	 * Use como exemplo a inicialização da fila no driver noop-iosched.c
	 *
	 */

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = nd;

	INIT_LIST_HEAD(&nd->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);

	return 0;
}

static void sstf_exit_queue(struct elevator_queue *e)
{
	struct sstf_data *nd = e->elevator_data;

	/* Implementação da finalização da fila (queue).
	 *
	 * Use como exemplo o driver noop-iosched.c
	 *
	 */
	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

/* Infrastrutura dos drivers de IO Scheduling. */
static struct elevator_type elevator_sstf = {
	.ops.sq = {
		.elevator_merge_req_fn		= sstf_merged_requests,
		.elevator_dispatch_fn		= sstf_dispatch,
		.elevator_add_req_fn		= sstf_add_request,
		.elevator_init_fn		= sstf_init_queue,
		.elevator_exit_fn		= sstf_exit_queue,
	},
	.elevator_name = "sstf",
	.elevator_owner = THIS_MODULE,
};

/* Inicialização do driver. */
static int __init sstf_init(void)
{
	return elv_register(&elevator_sstf);
}

/* Finalização do driver. */
static void __exit sstf_exit(void)
{
	elv_unregister(&elevator_sstf);
}

module_init(sstf_init);
module_exit(sstf_exit);

MODULE_AUTHOR("Miguel Xavier");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SSTF IO scheduler");
