--
-- PostgreSQL database dump
--

-- Dumped from database version 15.2
-- Dumped by pg_dump version 15.2

-- Started on 2024-11-07 15:49:18

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- TOC entry 217 (class 1259 OID 91396)
-- Name: PROJECT; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public."PROJECT" (
    "projectID" integer NOT NULL,
    name character varying(255) NOT NULL,
    "ownerID" integer NOT NULL
);


ALTER TABLE public."PROJECT" OWNER TO postgres;

--
-- TOC entry 218 (class 1259 OID 91410)
-- Name: PROJECT_MEMBER; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public."PROJECT_MEMBER" (
    "projectID" integer NOT NULL,
    "userID" integer NOT NULL
);


ALTER TABLE public."PROJECT_MEMBER" OWNER TO postgres;

--
-- TOC entry 216 (class 1259 OID 91395)
-- Name: PROJECT_projectID_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public."PROJECT_projectID_seq"
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."PROJECT_projectID_seq" OWNER TO postgres;

--
-- TOC entry 3369 (class 0 OID 0)
-- Dependencies: 216
-- Name: PROJECT_projectID_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public."PROJECT_projectID_seq" OWNED BY public."PROJECT"."projectID";


--
-- TOC entry 220 (class 1259 OID 91429)
-- Name: TASK; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public."TASK" (
    "taskID" integer NOT NULL,
    "projectID" integer NOT NULL,
    "userID" integer NOT NULL,
    name character varying(255) NOT NULL,
    description text,
    deadline timestamp without time zone NOT NULL,
    time_created timestamp without time zone DEFAULT CURRENT_TIMESTAMP
);


ALTER TABLE public."TASK" OWNER TO postgres;

--
-- TOC entry 219 (class 1259 OID 91428)
-- Name: TASK_taskID_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public."TASK_taskID_seq"
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."TASK_taskID_seq" OWNER TO postgres;

--
-- TOC entry 3370 (class 0 OID 0)
-- Dependencies: 219
-- Name: TASK_taskID_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public."TASK_taskID_seq" OWNED BY public."TASK"."taskID";


--
-- TOC entry 215 (class 1259 OID 91384)
-- Name: USER; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public."USER" (
    "userID" integer NOT NULL,
    name character varying(255) NOT NULL,
    email character varying(255) NOT NULL,
    password character varying(255) NOT NULL,
    time_created timestamp(0) without time zone DEFAULT CURRENT_TIMESTAMP NOT NULL
);


ALTER TABLE public."USER" OWNER TO postgres;

--
-- TOC entry 214 (class 1259 OID 91383)
-- Name: USER_userID_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public."USER_userID_seq"
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."USER_userID_seq" OWNER TO postgres;

--
-- TOC entry 3371 (class 0 OID 0)
-- Dependencies: 214
-- Name: USER_userID_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public."USER_userID_seq" OWNED BY public."USER"."userID";


--
-- TOC entry 3189 (class 2604 OID 91399)
-- Name: PROJECT projectID; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."PROJECT" ALTER COLUMN "projectID" SET DEFAULT nextval('public."PROJECT_projectID_seq"'::regclass);


--
-- TOC entry 3190 (class 2604 OID 91432)
-- Name: TASK taskID; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."TASK" ALTER COLUMN "taskID" SET DEFAULT nextval('public."TASK_taskID_seq"'::regclass);


--
-- TOC entry 3187 (class 2604 OID 91387)
-- Name: USER userID; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."USER" ALTER COLUMN "userID" SET DEFAULT nextval('public."USER_userID_seq"'::regclass);


--
-- TOC entry 3360 (class 0 OID 91396)
-- Dependencies: 217
-- Data for Name: PROJECT; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public."PROJECT" ("projectID", name, "ownerID") FROM stdin;
1	test project1	3
2	website redesign	3
3	testing2	3
\.


--
-- TOC entry 3361 (class 0 OID 91410)
-- Dependencies: 218
-- Data for Name: PROJECT_MEMBER; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public."PROJECT_MEMBER" ("projectID", "userID") FROM stdin;
1	3
2	3
3	3
\.


--
-- TOC entry 3363 (class 0 OID 91429)
-- Dependencies: 220
-- Data for Name: TASK; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public."TASK" ("taskID", "projectID", "userID", name, description, deadline, time_created) FROM stdin;
\.


--
-- TOC entry 3358 (class 0 OID 91384)
-- Dependencies: 215
-- Data for Name: USER; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public."USER" ("userID", name, email, password, time_created) FROM stdin;
5	thuan nguyen hoang	thuan@gmail.com	2042003	2024-11-04 21:33:25
3	thuan	thuan2@gmail.com	2042003	2024-11-03 21:19:30
10	anh le	anh.le@gmail.com	pass1234	2024-11-05 09:56:49
11	bao tran	bao.tran@gmail.com	passtwo	2024-11-05 09:56:49
12	cuong nguyen	cuong@gmail.com	pass3word	2024-11-05 09:56:49
13	dang pham	dang.pham@gmail.com	password	2024-11-05 09:56:49
14	linh hoang	linh.hoang@gmail.com	linhpass	2024-11-05 09:56:49
15	phuc	phuc@gmail.com	123456	2024-11-05 16:37:15
18	heooo	hhh@gmail.com	2042003	2024-11-05 22:21:35
19	jjjj	j@gmail.com	2042003	2024-11-06 16:48:39
20	haaa	k@gmail.com	2042003	2024-11-06 20:03:33
21	phuc pham	phuc1@gmail.com	123456	2024-11-07 10:40:33
\.


--
-- TOC entry 3372 (class 0 OID 0)
-- Dependencies: 216
-- Name: PROJECT_projectID_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public."PROJECT_projectID_seq"', 3, true);


--
-- TOC entry 3373 (class 0 OID 0)
-- Dependencies: 219
-- Name: TASK_taskID_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public."TASK_taskID_seq"', 1, true);


--
-- TOC entry 3374 (class 0 OID 0)
-- Dependencies: 214
-- Name: USER_userID_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public."USER_userID_seq"', 21, true);


--
-- TOC entry 3205 (class 2606 OID 91414)
-- Name: PROJECT_MEMBER PROJECT_MEMBER_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."PROJECT_MEMBER"
    ADD CONSTRAINT "PROJECT_MEMBER_pkey" PRIMARY KEY ("projectID", "userID");


--
-- TOC entry 3200 (class 2606 OID 91403)
-- Name: PROJECT PROJECT_name_key; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."PROJECT"
    ADD CONSTRAINT "PROJECT_name_key" UNIQUE (name);


--
-- TOC entry 3202 (class 2606 OID 91401)
-- Name: PROJECT PROJECT_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."PROJECT"
    ADD CONSTRAINT "PROJECT_pkey" PRIMARY KEY ("projectID");


--
-- TOC entry 3207 (class 2606 OID 91437)
-- Name: TASK TASK_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."TASK"
    ADD CONSTRAINT "TASK_pkey" PRIMARY KEY ("taskID");


--
-- TOC entry 3193 (class 2606 OID 91393)
-- Name: USER USER_email_key; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."USER"
    ADD CONSTRAINT "USER_email_key" UNIQUE (email);


--
-- TOC entry 3195 (class 2606 OID 91426)
-- Name: USER USER_name_key; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."USER"
    ADD CONSTRAINT "USER_name_key" UNIQUE (name);


--
-- TOC entry 3197 (class 2606 OID 91391)
-- Name: USER USER_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."USER"
    ADD CONSTRAINT "USER_pkey" PRIMARY KEY ("userID");


--
-- TOC entry 3208 (class 1259 OID 91448)
-- Name: idx_projectid; Type: INDEX; Schema: public; Owner: postgres
--

CREATE INDEX idx_projectid ON public."TASK" USING btree ("projectID");


--
-- TOC entry 3209 (class 1259 OID 91449)
-- Name: idx_userid; Type: INDEX; Schema: public; Owner: postgres
--

CREATE INDEX idx_userid ON public."TASK" USING btree ("userID");


--
-- TOC entry 3203 (class 1259 OID 91404)
-- Name: project_projectid_index; Type: INDEX; Schema: public; Owner: postgres
--

CREATE INDEX project_projectid_index ON public."PROJECT" USING btree ("projectID");


--
-- TOC entry 3198 (class 1259 OID 91394)
-- Name: user_userid_index; Type: INDEX; Schema: public; Owner: postgres
--

CREATE INDEX user_userid_index ON public."USER" USING btree ("userID");


--
-- TOC entry 3213 (class 2606 OID 91438)
-- Name: TASK fk_project; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."TASK"
    ADD CONSTRAINT fk_project FOREIGN KEY ("projectID") REFERENCES public."PROJECT"("projectID") ON DELETE CASCADE;


--
-- TOC entry 3214 (class 2606 OID 91443)
-- Name: TASK fk_user; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."TASK"
    ADD CONSTRAINT fk_user FOREIGN KEY ("userID") REFERENCES public."USER"("userID") ON DELETE SET NULL;


--
-- TOC entry 3211 (class 2606 OID 91420)
-- Name: PROJECT_MEMBER project_member_projectid_foreign; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."PROJECT_MEMBER"
    ADD CONSTRAINT project_member_projectid_foreign FOREIGN KEY ("projectID") REFERENCES public."PROJECT"("projectID") ON DELETE CASCADE;


--
-- TOC entry 3212 (class 2606 OID 91415)
-- Name: PROJECT_MEMBER project_member_userid_foreign; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."PROJECT_MEMBER"
    ADD CONSTRAINT project_member_userid_foreign FOREIGN KEY ("userID") REFERENCES public."USER"("userID") ON DELETE CASCADE;


--
-- TOC entry 3210 (class 2606 OID 91405)
-- Name: PROJECT project_ownerid_foreign; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public."PROJECT"
    ADD CONSTRAINT project_ownerid_foreign FOREIGN KEY ("ownerID") REFERENCES public."USER"("userID");


-- Completed on 2024-11-07 15:49:19

--
-- PostgreSQL database dump complete
--

