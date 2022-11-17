# サポートするコードバターン

## Synchronization

- [x] `pthread_create`
- [x] `pthread_join`
- [x] `pthread_mutex_lock`
- [x] `pthread_mutex_unlock`

### (おそらく)サポート

- [ ] `pthread_rwlock_rdlock`
- [ ] `pthread_rwlock_unlock`

- [ ] `sem_wait`
- [ ] `sem_post`

- [ ] `_spin_lock`
- [ ] `_spin_unlock`


### サポート対象外

- [ ] `pthread_barrier_wait`
- [ ] `__atomic_*`

- [ ] `sleep`
