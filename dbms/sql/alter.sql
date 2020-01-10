alter table test add ddd int;
alter table test add index test(a);
alter table test drop index test;
alter table test add primary key(a, b);
alter table test drop primary key;
alter table test add constraint test primary key (a, b, c);
alter table test drop primary key test;
desc test;