create database small;
use small;
CREATE TABLE PART (
	P_PARTKEY		INT,
	P_NAME			VARCHAR(55),
	P_MFGR			CHAR(25),
	P_BRAND			CHAR(10),
	P_TYPE			VARCHAR(25),
	P_SIZE			INT,
	P_CONTAINER		CHAR(10),
	P_RETAILPRICE	FLOAT,
	P_COMMENT		VARCHAR(23),
	PRIMARY KEY(P_PARTKEY)
);

CREATE TABLE REGION (
	R_REGIONKEY	INT,
	R_NAME		CHAR(25),
	R_COMMENT	VARCHAR(152),
	PRIMARY KEY(R_REGIONKEY)
);

CREATE TABLE NATION (
	N_NATIONKEY		INT,
	N_NAME			CHAR(25),
	N_REGIONKEY		INT NOT NULL,
	N_COMMENT		VARCHAR(152),
	PRIMARY KEY(N_NATIONKEY)
);
CREATE TABLE SUPPLIER (
	S_SUPPKEY		INT,
	S_NAME			CHAR(25),
	S_ADDRESS		VARCHAR(40),
	S_NATIONKEY		INT NOT NULL,
	S_PHONE			CHAR(15),
	S_ACCTBAL		FLOAT,
	S_COMMENT		VARCHAR(101),
	PRIMARY KEY(S_SUPPKEY)
);

CREATE TABLE CUSTOMER (
	C_CUSTKEY		INT,
	C_NAME			VARCHAR(25),
	C_ADDRESS		VARCHAR(40),
	C_NATIONKEY		INT NOT NULL,
	C_PHONE			CHAR(15),
	C_ACCTBAL		FLOAT,
	C_MKTSEGMENT	CHAR(10),
	C_COMMENT		VARCHAR(117),
	PRIMARY KEY(C_CUSTKEY)
);

CREATE TABLE PARTSUPP (
	PS_PARTKEY		INT NOT NULL,
	PS_SUPPKEY		INT NOT NULL,
	PS_AVAILQTY		INT,
	PS_SUPPLYCOST	FLOAT,
	PS_COMMENT		VARCHAR(199),
	PRIMARY KEY (PS_PARTKEY, PS_SUPPKEY)
);
CREATE TABLE ORDERS (
	O_ORDERKEY		INT,
	O_CUSTKEY		INT NOT NULL,
	O_ORDERSTATUS	CHAR(1),
	O_TOTALPRICE	FLOAT,
	O_ORDERDATE		DATE,
	O_ORDERPRIORITY	CHAR(15),
	O_CLERK			CHAR(15),
	O_SHIPPRIORITY	INT,
	O_COMMENT		VARCHAR(79),
	PRIMARY KEY(O_ORDERKEY)
);
CREATE TABLE LINEITEM (
	L_ORDERKEY		INT NOT NULL,
	L_PARTKEY		INT NOT NULL,
	L_SUPPKEY		INT NOT NULL,
	L_LINENUMBER	INT,
	L_QUANTITY		FLOAT,
	L_EXTENDEDPRICE	FLOAT,
	L_DISCOUNT		FLOAT,
	L_TAX			FLOAT,
	L_RETURNFLAG	CHAR(1),
	L_LINESTATUS	CHAR(1),
	L_SHIPDATE		DATE,
	L_COMMITDATE	DATE,
	L_RECEIPTDATE	DATE,
	L_SHIPINSTRUCT	CHAR(25),
	L_SHIPMODE		CHAR(10),
	L_COMMENT		VARCHAR(44),
	PRIMARY KEY (L_ORDERKEY, L_LINENUMBER)  
);

ALTER TABLE SUPPLIER ADD constraint fk FOREIGN KEY (S_NATIONKEY) REFERENCES NATION(N_NATIONKEY);

ALTER TABLE PARTSUPP ADD constraint fk FOREIGN KEY (PS_PARTKEY) REFERENCES PART(P_PARTKEY);
ALTER TABLE PARTSUPP ADD constraint fk2 FOREIGN KEY (PS_SUPPKEY) REFERENCES SUPPLIER(S_SUPPKEY);

ALTER TABLE CUSTOMER ADD constraint fk FOREIGN KEY (C_NATIONKEY) REFERENCES NATION(N_NATIONKEY);

ALTER TABLE ORDERS ADD constraint fk FOREIGN KEY (O_CUSTKEY) REFERENCES CUSTOMER(C_CUSTKEY);

ALTER TABLE LINEITEM ADD constraint fk FOREIGN KEY (L_ORDERKEY) REFERENCES ORDERS(O_ORDERKEY);
ALTER TABLE LINEITEM ADD constraint fk2 FOREIGN KEY (L_PARTKEY,L_SUPPKEY) REFERENCES PARTSUPP(PS_PARTKEY,PS_SUPPKEY);

ALTER TABLE NATION ADD constraint fk FOREIGN KEY (N_REGIONKEY) REFERENCES REGION(R_REGIONKEY);
